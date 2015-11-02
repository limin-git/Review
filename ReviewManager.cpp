#include "stdafx.h"
#include "ReviewManager.h"
#include "History.h"
#include "Loader.h"
#include "Speech.h"
#include "ReviewString.h"
#include "Utility.h"
#include "Log.h"
#include "OptionString.h"
#include "ProgramOptions.h"
#include "OptionUpdateHelper.h"


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
            const std::string& ls = loader->get_string_no_lock( lhs );
            const std::string& rs = loader->get_string_no_lock( rhs );
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


ReviewManager::ReviewManager()
    : m_review_mode( Forward ),
      m_backward_index( 0 ),
      m_loader( NULL ),
      m_history( NULL ),
      m_speech( NULL ),
      m_speech_impl( NULL ),
      m_is_listening( false ),
      m_minimal_review_time( 500 * 1000 * 1000 ),
      m_auto_update_interval( 60 ),
      m_play_back( 0 ),
      m_current_reviewing( NULL ),
      m_listen_no_string( false ),
      m_listen_all( false ),
      m_answer_first( false )
{
    m_loader = new Loader;
    m_history = new History;
    m_speech_impl = new Speech;
    ProgramOptions::connect_to_signal( boost::bind( &ReviewManager::update_option, this, _1 ) );
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
    update();
    new boost::thread( boost::bind( &ReviewManager::update_thread, this ) );

    while ( true )
    {
        p = n;
        n = get_next();
        m_current_reviewing = &n;

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
                m_current_reviewing = &p;
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

            if ( c == "delete" || c == "d" )
            {
                m_history->save_history( n.get_hash(), 0 );
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

                std::vector< std::pair<std::string, std::string> > word_paths = m_speech->get_word_speech_file_path( w );
                Utility::play_or_tts_list_thread( word_paths );
            }

            m_current_reviewing = NULL;
            LOG_TRACE << "end do";
        }
        while ( t.elapsed().wall < m_minimal_review_time );
    }
}


ReviewString ReviewManager::get_next()
{
    LOG_TRACE << "begin";

    boost::unique_lock<boost::mutex> lock( m_mutex );

    m_review_mode = Forward;

    if ( m_reviewing_list.empty() )
    {
        update();

        if ( m_reviewing_list.empty() )
        {
            return ReviewString();
        }
    }

    size_t hash = get_next_hash( m_reviewing_list, get_next_order( m_review_orders, m_review_order_it ) );
    m_reviewing_set.erase( hash );
    set_title();
    m_review_history.push_back( hash );
    m_history->save_history( hash, std::time(0) );

    if ( m_reviewing_set.empty() )
    {
        m_history->write_history();
        m_history->clean_review_cache();
    }

    m_condition.notify_all();

    LOG_TRACE << "end";
    return ReviewString( hash, m_loader, m_history, m_speech, m_display_format );
}


ReviewString ReviewManager::get_previous()
{
    boost::unique_lock<boost::mutex> lock( m_mutex );

    if ( m_review_history.empty() )
    {
        return ReviewString();
    }

    if ( m_review_mode == Forward )
    {
        m_review_mode = Backward;

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

    return ReviewString( m_review_history[m_backward_index], m_loader, m_history, m_speech, m_display_format );
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
    input.erase( std::remove_if( input.begin(), input.end(), boost::is_any_of("\\[]+-") ), input.end() );
    return input;
}


void ReviewManager::set_title()
{
    std::stringstream strm;
    strm << "TITLE " << m_file_name << " - " << m_reviewing_set.size();
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
        if ( ! m_reviewing_set.empty() )
        {
            LOG_DEBUG
                << "old-size=" << m_reviewing_list.size()
                << ", new-size=" << expired.size() << ""
                << get_new_expired_string( m_reviewing_set, expired )
                ;
        }
        else
        {
            LOG_DEBUG
                << "old-size=" << m_reviewing_list.size()
                << ", new-size=" << expired.size() << ""
                ;
        }

        m_reviewing_set = expired;
        m_reviewing_list.assign( m_reviewing_set.begin(), m_reviewing_set.end() );

        {
            static Order order(m_loader, m_history);
            boost::unique_lock<boost::mutex> lock( m_loader->m_mutex );
            m_reviewing_list.sort( order );
        }

        set_title();
        LOG_DEBUG << "sort " << m_reviewing_list.size();
        LOG_TEST<< std::endl << get_hash_list_string( m_reviewing_list );
    }

    LOG_TRACE << "end";
}


void ReviewManager::update_thread()
{
    boost::chrono::seconds interval( m_auto_update_interval );

    while ( true )
    {
        boost::unique_lock<boost::mutex> lock( m_mutex );
        m_condition.wait_for( lock, interval );

        {
            LOG_TRACE << "begin";
            LOG_DEBUG << "update thread";
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

    std::list<size_t> listen_list;

    if ( m_listen_all )
    {
        listen_list.assign( m_all.begin(), m_all.end() );
    }
    else
    {
        listen_list = m_reviewing_list;
    }

    std::vector<EReviewOrder> listen_orders = m_review_orders;
    std::vector<EReviewOrder>::iterator listen_orders_it = listen_orders.begin();

    while ( m_is_listening && ! listen_list.empty() )
    {
        size_t hash = get_next_hash( listen_list, get_next_order( listen_orders, listen_orders_it ) );
        const std::string& s = m_loader->get_string( hash );
        std::vector<std::string> words = Utility::extract_words( s );

        if ( words.empty() )
        {
            continue;
        }

        if ( m_speech == NULL )
        {
            break;
        }

        std::vector< std::pair<std::string, std::string> > word_paths = m_speech->get_word_speech_file_path( words );

        if ( word_paths.empty() )
        {
            continue;
        }

        system( "CLS" );
        system( ( "TITLE listen - " + boost::lexical_cast<std::string>( listen_list.size() ) ).c_str() );

        if ( ! m_listen_no_string )
        {
            std::cout << "\t" << s << std::endl;
        }

        std::cout << "\t";
        std::copy( words.begin(), words.end(), std::ostream_iterator<std::string>( std::cout, "\n\t" ) );
        LOG_TRACE << s;
        Utility::play_or_tts_list( word_paths );
    }

    set_title();
}


void ReviewManager::update_option( const boost::program_options::variables_map& vm )
{
    static OptionUpdateHelper option_helper;

    if ( option_helper.update_one_option<boost::timer::nanosecond_type>( review_minimal_time_option, vm, 500 ) )
    {
        m_minimal_review_time = option_helper.get_value<boost::timer::nanosecond_type>( review_minimal_time_option ) * 1000 * 1000;
        LOG_DEBUG << "review-minimal-review-time: " << m_minimal_review_time;
    }

    if ( option_helper.update_one_option<size_t>( review_auto_update_interval_option, vm, 60 ) )
    {
        m_auto_update_interval = option_helper.get_value<size_t>( review_auto_update_interval_option );
        LOG_DEBUG << "review-auto-update-interval: " << m_auto_update_interval;
    }

    if ( option_helper.update_one_option<size_t>( speech_play_back, vm, 0 ) )
    {
        m_play_back = option_helper.get_value<size_t>( speech_play_back );
        LOG_DEBUG << "speech-play-back: " << m_play_back;
    }

    if ( option_helper.update_one_option<std::string>( review_order_option, vm, "latest" ) )
    {
        boost::unique_lock<boost::mutex> lock( m_mutex );
        std::string order_option_str = option_helper.get_value<std::string>( review_order_option );
        m_review_orders = convert_from_string( order_option_str );
        m_review_order_it = m_review_orders.begin();
        LOG_DEBUG << "review-order: " << order_option_str;
    }

    if ( option_helper.update_one_option<std::string>( speech_disabled_option, vm, "false" ) )
    {
        bool speech_disabled = ( "true" == option_helper.get_value<std::string>( speech_disabled_option ) );

        if ( speech_disabled )
        {
            m_speech = NULL;
            if ( m_current_reviewing != NULL ) { m_current_reviewing->m_speech = NULL; }
            LOG_DEBUG << "speech-disabled: true";
        }
        else
        {
            m_speech = m_speech_impl;
            if ( m_current_reviewing != NULL ) { m_current_reviewing->m_speech = m_speech; }
            LOG_DEBUG << "speech-disabled: false";
        }
    }

    if ( option_helper.update_one_option<std::string>( listen_no_string_option, vm, "false" ) )
    {
        m_listen_no_string = ( "true" == option_helper.get_value<std::string>( listen_no_string_option ) );
        LOG_DEBUG << "listen-no-string: " << ( m_listen_no_string ? "true" : "false" );
    }

    if ( option_helper.update_one_option<std::string>( listen_all_option, vm, "false" ) )
    {
        m_listen_all = ( "true" == option_helper.get_value<std::string>( listen_all_option ) );
        LOG_DEBUG << "listen-all: " << ( m_listen_all ? "true" : "false" );
    }

    if ( option_helper.update_one_option<std::string>( review_answer_first, vm, "false" ) )
    {
        m_answer_first = ( "true" == option_helper.get_value<std::string>( review_answer_first ) );
        LOG_DEBUG << "review-answer-first: " << m_answer_first;
    }

    if ( option_helper.update_one_option<std::string>( file_name_option, vm ) )
    {
        m_file_name = option_helper.get_value<std::string>( file_name_option );
        LOG_DEBUG << "file-name: " << m_file_name;
    }

    if ( option_helper.update_one_option<std::string>( review_display_format, vm, "Q,AE" ) )
    {
        m_display_format = option_helper.get_value<std::string>( review_display_format );
        LOG_DEBUG << "review-display-format: " << m_display_format;
    }    
}


size_t ReviewManager::get_next_hash( std::list<size_t>& hash_list, EReviewOrder order )
{
    if ( hash_list.empty() )
    {
        return 0;
    }

    size_t hash = 0;

    if ( order == Latest )
    {
        hash = hash_list.front();
        hash_list.pop_front();
    }
    else if ( order == Earliest )
    {
        hash = hash_list.back();
        hash_list.pop_back();
    }
    else if ( order == Random )
    {
        if ( hash_list.size() < 3 )
        {
            hash = hash_list.back();
            hash_list.pop_back();
        }
        else
        {
            std::list<size_t>::iterator it = hash_list.begin();
            std::advance( it, Utility::random_number( 1, hash_list.size() - 2 ) );
            hash = *it;
            hash_list.erase( it );
        }
    }
    else
    {
        LOG << "error: the review order is unknown: " << order;
    }

    return hash;
}


ReviewManager::EReviewOrder ReviewManager::get_next_order( std::vector<EReviewOrder>& orders, std::vector<EReviewOrder>::iterator& it )
{
    if ( it == orders.end() )
    {
        it = orders.begin();
    }

    if ( it == orders.end() )
    {
        return Invalide;
    }

    EReviewOrder order = *it;
    it++;

    static const char* order_str[] = { "Latest", "Earliest", "Random" };
    LOG_TRACE << "review order: " << order_str[order];
    return order;
}


std::vector<ReviewManager::EReviewOrder> ReviewManager::convert_from_string( const std::string& order_string )
{
    std::vector<ReviewManager::EReviewOrder> orders;
    std::vector<std::string> vos;
    boost::split( vos, order_string, boost::is_any_of( ",:;-/\\%@|" ) );

    for ( size_t i = 0; i < vos.size(); ++i )
    {
        boost::to_lower(vos[i]);
        if ( vos[i] == "latest" ) { orders.push_back( Latest ); }
        else if ( vos[i] == "earlist" ) { orders.push_back( Earliest ); }
        else if ( vos[i] == "random" ) { orders.push_back( Random ); }
    }

    return orders;
}


void ReviewManager::upgrade_hash_algorithm()
{
    std::cout << "upgrading hash algorithm for " << m_loader->m_file_name << " ..." << std::flush;

    m_history->initialize();

    history_type& old_history = m_history->m_history;
    boost::function<size_t (const std::string&)> new_hash_fun = &Loader::string_hash_new;
    const std::set<size_t>& hash_set = m_loader->get_string_hash_set();
    history_type new_history;

    for ( std::set<size_t>::const_iterator it = hash_set.begin(); it != hash_set.end(); ++it )
    {
        size_t hash = *it;
        const std::string& s = m_loader->get_string_no_lock( hash );

        history_type::iterator find_it = old_history.find( hash );

        if ( find_it == old_history.end() )
        {
            std::cout << "\naborted: not match (updated already?)" << std::endl;
            return;
        }

        size_t new_hash = new_hash_fun( s );
        new_history[new_hash] = find_it->second;
    }

    if ( old_history == new_history )
    {
        std::cout << "\nthe algorithm is same, do nothing." << std::endl;
        return;
    }

    m_history->m_history = new_history;
    m_history->write_history();
    std::cout << "\ndone." << std::endl;
}
