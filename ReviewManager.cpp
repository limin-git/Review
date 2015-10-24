#include "stdafx.h"
#include "ReviewManager.h"
#include "History.h"
#include "Loader.h"
#include "Speech.h"
#include "ReviewString.h"
#include "Utility.h"
#include "Log.h"
#include "OptionString.h"
#include "ConfigFileMonitor.h"


struct Order
{
    Order( Loader* l, History* h ) : loader(l), history(h) {}

    bool operator()( size_t lhs, size_t rhs ) const
    {
        size_t lr = history->get_review_round(lhs);
        size_t rr = history->get_review_round(rhs);
        std::time_t lt = history->get_last_review_time( lhs );
        std::time_t rt = history->get_last_review_time( rhs );

        if ( ( lt < rr ) || ( lr == rr && rt < lt ) )
        {
            return true;
        }
        else
        {
            const std::string& ls = loader->get_string( lhs );
            const std::string& rs = loader->get_string( rhs );
            return
                ( lr < rr ) ||
                ( lr == rr && rt < lt ) ||
                ( lr == rr && lt == rt && ls.size() < rs.size() ) ||
                ( lr == rr && lt == rt && ls.size() == rs.size() && ls < rs );
        }
    }

    Loader* loader;
    History* history;
};


ReviewManager::ReviewManager( const boost::program_options::variables_map& vm )
    : m_variables_map( vm ),
      m_review_mode( forward ),
      m_backward_index( 0 ),
      m_loader( NULL ),
      m_history( NULL ),
      m_speech( NULL ),
      m_speech_impl( NULL ),
      m_is_listening( false ),
      m_minimal_review_time( 500 * 1000 * 1000 ),
      m_auto_update_interval( 60 ),
      m_play_back( 0 )
{
    m_loader = new Loader( vm );
    m_history = new History( vm );
    m_speech_impl = new Speech( vm );
    update_option( vm );
    ConfigFileMonitor::connect_to_signal( boost::bind( &ReviewManager::update_option, this, _1 ) );
}


void ReviewManager::review()
{
    std::string c;
    boost::timer::cpu_timer t;
    ReviewString n;
    ReviewString p;
    std::list<ReviewString> play_back;

    m_history->initialize();
    m_history->synchronize_history( m_loader->get_string_hash_set() );
    new boost::thread( boost::bind( &ReviewManager::update_thread, this ) );

    while ( true )
    {
        ConfigFileMonitor::scan_file();

        p = n;
        n = get_next();

        do
        {
            LOG_TRACE << "begin do";

            t.start();
            c = n.review();

            if ( c.empty() )
            {
                c = wait_for_input();
            }

            while ( c == "previous" || c == "p" || c == "back" || c == "b" )
            {
                system( "CLS" );

                p = get_previous();
                c = p.review();

                if ( c.empty() )
                {
                    c = wait_for_input();
                }
            }

            if ( c == "listen" || c == "l" )
            {
                m_is_listening = true;
                boost::thread t( boost::bind( &ReviewManager::listen_thread, this ) );
                c = wait_for_input();
                m_is_listening = false;
            }

            if ( m_speech && m_play_back )
            {
                play_back.push_back( n );

                while ( m_play_back < play_back.size() )
                {
                    play_back.pop_front();
                }

                std::vector<std::string> w;

                for ( std::list<ReviewString>::iterator it = play_back.begin(); it != play_back.end(); ++it )
                {
                    std::vector<std::string> w2 = Utility::extract_words( it->get_string() );
                    w.insert( w.end(), w2.begin(), w2.end() );
                }

                std::vector<std::string> files = m_speech->get_files( w );
                Utility::play_sound_thread( files );
            }

            LOG_TRACE << "end do";
        }
        while ( t.elapsed().wall < m_minimal_review_time );
    }
}


ReviewString ReviewManager::get_next()
{
    LOG_TRACE << "begin";

    boost::unique_lock<boost::mutex> lock( m_mutex );

    m_review_mode = forward;

    update();

    if ( m_reviewing_list.empty() )
    {
        return ReviewString();
    }

    size_t hash = m_reviewing_list.front();
    m_reviewing_list.pop_front();
    m_reviewing_set.erase( hash );
    set_title();
    m_review_history.push_back( hash );
    m_history->save_history( hash );

    if ( m_reviewing_set.empty() )
    {
        m_history->write_history();
        m_history->clean_review_cache();
    }

    LOG_TRACE << "end";
    return ReviewString( hash, m_loader, m_history, m_speech );
}


ReviewString ReviewManager::get_previous()
{
    boost::unique_lock<boost::mutex> lock( m_mutex );

    if ( m_review_history.empty() )
    {
        return ReviewString();
    }

    if ( m_review_mode == forward )
    {
        m_review_mode = backward;

        if ( m_review_history.size() == 1 )
        {
            m_backward_index = 0;
        }
        else
        {
            m_backward_index = m_review_history.size() - 1;
        }
    }

    if ( 0 < m_backward_index )
    {
        m_backward_index--;
    }

    return ReviewString( m_review_history[m_backward_index], m_loader, m_history, m_speech );
}


std::string ReviewManager::wait_for_input( const std::string& message )
{
    if ( ! message.empty() )
    {
        std::cout << message << std::flush;
    }

    std::string input;
    std::getline( std::cin, input );
    system( "CLS" );
    boost::trim(input);
    return input;
}


void ReviewManager::set_title()
{
    std::stringstream strm;
    strm << "TITLE " << m_reviewing_set.size();
    system( strm.str().c_str() );
}


void ReviewManager::update()
{
    LOG_TRACE << "begin";

    const std::set<size_t>& all = m_loader->get_string_hash_set();

    if ( m_all != all )
    {
        m_all = all;
        m_history->synchronize_history( m_all );
    }

    std::set<size_t> expired = m_history->get_expired();

    if ( m_reviewing_set != expired )
    {
        LOG_DEBUG
            << "old-size=" << m_reviewing_list.size()
            << ", new-size=" << expired.size() << ""
            << get_new_expired_string( m_reviewing_set, expired )
            ;

        m_reviewing_set = expired;
        m_reviewing_list.assign( m_reviewing_set.begin(), m_reviewing_set.end() );

        static Order order(m_loader, m_history);
        m_reviewing_list.sort( order );
        set_title();
        LOG_DEBUG << "sort " << m_reviewing_list.size();
        LOG_TEST<< std::endl << get_hash_list_string( m_reviewing_list );
    }

    LOG_TRACE << "end";
}


void ReviewManager::update_thread()
{
    boost::chrono::seconds wait_for( m_auto_update_interval );

    while ( true )
    {
        boost::this_thread::sleep_for( wait_for );

        {
            LOG_TRACE << "begin";
            LOG_DEBUG << "update thread";
            boost::unique_lock<boost::mutex> lock( m_mutex );
            update();
            LOG_TRACE << "end";
        }
    }
}


std::ostream& ReviewManager::output_hash_list( std::ostream& os, const std::list<size_t>& l )
{
    for ( std::list<size_t>::const_iterator it = l.begin(); it != l.end(); ++it )
    {
        size_t hash = *it;
        std::time_t t = m_history->get_last_review_time( hash );
        size_t r = m_history->get_review_round( hash );
        const std::string& s = m_loader->get_string( hash );

        os 
            << r << "\t"
            << Utility::time_string( t ) << "\t"
            << s.size() << "\t"
            << s << "\n";
            ;
    }

    return os;
}


std::string ReviewManager::get_hash_list_string( const std::list<size_t>& l )
{
    std::stringstream strm;
    output_hash_list( strm, l );
    return strm.str();
}


std::string ReviewManager::get_new_expired_string( const std::set<size_t>& os,  const std::set<size_t>& ns )
{
    std::set<size_t> added;
    std::set_difference( ns.begin(), ns.end(), os.begin(), os.end(), std::inserter( added, added.begin() ) );

    std::stringstream strm;

    for ( std::set<size_t>::iterator it = added.begin(); it != added.end(); ++it )
    {
        size_t hash = *it;
        size_t round = m_history->get_review_round( hash );
        std::time_t last_review = m_history->get_last_review_time( hash );
        std::time_t elapsed = std::time(0) - last_review;
        const std::string& s = m_loader->get_string( *it );
        strm << std::endl << "expired: " << round << " (" << Utility::time_duration_string(elapsed) << ") " << s;
    }

    return strm.str();
}


void ReviewManager::listen_thread()
{
    update();

    std::vector<size_t> listen_list( m_reviewing_list.begin(), m_reviewing_list.end() );

    for ( size_t i = 0; i < listen_list.size() && m_is_listening; ++i )
    {
        size_t hash = listen_list[i];
        const std::string& s = m_loader->get_string( hash );
        std::vector<std::string> words = Utility::extract_words( s );

        if ( words.empty() )
        {
            continue;
        }

        ConfigFileMonitor::scan_file();

        if ( m_speech == NULL )
        {
            break;
        }

        std::vector<std::string> files = m_speech->get_files( words );

        if ( files.empty() )
        {
            continue;
        }

        system( "CLS" );
        system( ( "TITLE listen - " + boost::lexical_cast<std::string>( listen_list.size() - i ) ).c_str() );
        std::cout << s << std::endl;
        std::copy( words.begin(), words.end(), std::ostream_iterator<std::string>( std::cout, "\n" ) );
        Utility::play_sounds( files );
    }

    set_title();
}


void ReviewManager::update_option( const boost::program_options::variables_map& vm )
{
    boost::timer::nanosecond_type minimal_review_time = vm[review_minimal_time_option].as<boost::timer::nanosecond_type>() * 1000 * 1000;
    size_t auto_update_interval = vm[review_auto_update_interval_option].as<size_t>();
    size_t play_back = vm[speech_play_back].as<size_t>();

    if ( m_minimal_review_time != minimal_review_time )
    {
        m_minimal_review_time = minimal_review_time;
        LOG_DEBUG << "m_minimal_review_time = " << m_minimal_review_time;
    }

    if ( m_auto_update_interval != auto_update_interval )
    {
        m_auto_update_interval = auto_update_interval;
        LOG_DEBUG << "m_auto_update_interval = " << m_auto_update_interval;
    }

    if ( m_play_back != play_back )
    {
        m_play_back = play_back;
        LOG_DEBUG << "m_play_back = " << m_play_back;
    }

    if ( ( ! vm.count( speech_disabled_option ) ) || ( vm[speech_disabled_option].as<std::string>() != "true" ) )
    {
        if ( m_speech == NULL )
        {
            m_speech = m_speech_impl;
            LOG_DEBUG << "new speech";
        }
    }
    else
    {
        if ( m_speech != NULL )
        {
            m_speech = NULL;
            LOG_DEBUG << "delete speech";
        }
    }
}
