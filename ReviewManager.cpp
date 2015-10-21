#include "stdafx.h"
#include "ReviewManager.h"
#include "History.h"
#include "Loader.h"
#include "Speech.h"
#include "ReviewString.h"
#include "Utility.h"


extern boost::log::sources::logger m_log_debug;
extern boost::log::sources::logger m_log_trace;
extern boost::log::sources::logger m_log_test;


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
      m_backward_index( 0 )
{
    m_minimal_review_time = vm["minimal-review-time"].as<boost::timer::nanosecond_type>() * 1000 * 1000;
    m_auto_update_interval = vm["auto-update-interval"].as<size_t>();
    m_loader = new Loader( vm );
    m_history = new History( vm );
    m_speech = new Speech( vm );
}


void ReviewManager::review()
{
    std::string c;
    boost::timer::cpu_timer t;

    m_history->initialize();
    m_history->synchronize_history( m_loader->get_string_hash_set() );
    new boost::thread( boost::bind( &ReviewManager::update_thread, this ) );

    while ( true )
    {
        ReviewString s = get_next();

        do
        {
            BOOST_LOG(m_log_trace) << __FUNCTION__ << " - begin do";

            t.start();
            c = s.review();

            if ( c.empty() )
            {
                c = wait_for_input();
            }

            while ( c == "previous" || c == "p" || c == "back" || c == "b" )
            {
                system( "CLS" );
                c = get_previous().review();

                if ( c.empty() )
                {
                    c = wait_for_input();
                }
            }

            while ( c == "speak" || c == "speech" || c == "s" )
            {
                system( "CLS" );
                s.speech();
                c = wait_for_input();
            }

            BOOST_LOG(m_log_trace) << __FUNCTION__ << " - end do";
        }
        while ( t.elapsed().wall < m_minimal_review_time );
    }
}


ReviewString ReviewManager::get_next()
{
    BOOST_LOG(m_log_trace) << __FUNCTION__ << " - begin";

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

    BOOST_LOG(m_log_trace) << __FUNCTION__ << " - end";
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

    return ReviewString( m_review_history[m_backward_index], m_loader, m_history );
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
    BOOST_LOG(m_log_trace) << __FUNCTION__ << " - begin";

    const std::set<size_t>& all = m_loader->get_string_hash_set();

    if ( m_all != all )
    {
        m_all = all;
        m_history->synchronize_history( m_all );
    }

    std::set<size_t> expired = m_history->get_expired();

    if ( m_reviewing_set != expired )
    {
        BOOST_LOG(m_log_debug) << __FUNCTION__ << " - "
            << "old-size=" << m_reviewing_list.size()
            << ", new-size=" << expired.size() << ""
            << get_new_expired_string( m_reviewing_set, expired )
            ;
        m_reviewing_set = expired;
        m_reviewing_list.assign( m_reviewing_set.begin(), m_reviewing_set.end() );

        static Order order(m_loader, m_history);
        m_reviewing_list.sort( order );
        set_title();
        BOOST_LOG(m_log_debug) << __FUNCTION__ << " - sort " << m_reviewing_list.size();
        BOOST_LOG(m_log_test) << __FUNCTION__ << ":" << std::endl << get_hash_list_string( m_reviewing_list );
    }

    BOOST_LOG(m_log_trace) << __FUNCTION__ << " - end";
}


void ReviewManager::update_thread()
{
    boost::chrono::seconds wait_for( m_auto_update_interval );

    while ( true )
    {
        boost::this_thread::sleep_for( wait_for );

        {
            BOOST_LOG(m_log_trace) << __FUNCTION__ << " - begin";
            BOOST_LOG(m_log_debug) << __FUNCTION__;
            boost::unique_lock<boost::mutex> lock( m_mutex );
            update();
            BOOST_LOG(m_log_trace) << __FUNCTION__ << " - end";
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
