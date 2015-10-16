#include "stdafx.h"
#include "ReviewManager.h"
#include "History.h"
#include "Loader.h"
#include "ReviewString.h"


struct Order
{
    Order( Loader* l, History* h ) : loader(l), history(h) {}

    bool operator()( size_t lhs, size_t rhs )
    {
        size_t lr = history->get_review_round(lhs);
        size_t rr = history->get_review_round(rhs);
        std::time_t lt = history->get_last_review_time( lhs );
        std::time_t rt = history->get_last_review_time( rhs );
        const std::string& ls = loader->get_string( lhs );
        const std::string& rs = loader->get_string( rhs );
        return
            ( lr < rr ) ||
            ( lr == rr && rt < lt ) ||
            ( lr == rr && lt == rt && ls.size() < rs.size() ) ||
            ( lr == rr && lt == rt && ls.size() == rs.size() && ls < rs );
    }

    Loader* loader;
    History* history;
};


ReviewManager::ReviewManager( const boost::program_options::variables_map& vm )
    : m_variables_map( vm )
{
    m_minimal_review_time = vm["minimal-review-time"].as<size_t>() * 1000 * 1000;
    m_auto_update_interval = vm["auto-update-interval"].as<size_t>();
    m_loader = new Loader( vm );
    m_history = new History( vm );
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
            t.start();
            c = s.review();
            c = wait_for_input();
        }
        while ( t.elapsed().wall < m_minimal_review_time );
    }
}


ReviewString ReviewManager::get_next()
{
    boost::unique_lock<boost::mutex> lock( m_mutex );

    update();

    if ( m_reviewing_list.empty() )
    {
        return ReviewString();;
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

    return ReviewString( hash, m_loader, m_history );
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
    const std::set<size_t>& all = m_loader->get_string_hash_set();

    if ( m_all != all )
    {
        m_all = all;
        m_history->synchronize_history( m_all );
    }

    std::set<size_t> expired = m_history->get_expired();

    if ( m_reviewing_set != expired )
    {
        m_reviewing_set = expired;
        m_reviewing_list.clear();
        m_reviewing_list.assign( m_reviewing_set.begin(), m_reviewing_set.end() );

        static Order order(m_loader, m_history);
        m_reviewing_list.sort( order );
        set_title();
    }
}


void ReviewManager::update_thread()
{
    boost::chrono::seconds wait_for( m_auto_update_interval );

    while ( true )
    {
        boost::this_thread::sleep_for( wait_for );

        {
            boost::unique_lock<boost::mutex> lock( m_mutex );
            update();
        }
    }
}
