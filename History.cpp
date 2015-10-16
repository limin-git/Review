#include "stdafx.h"
#include "History.h"
#include "Utility.h"


History::History( const boost::program_options::variables_map& vm )
    : m_variables_map( vm ),
      m_max_cache_size( 100 ),
      m_cache_size( 0 )
{
    std::string name = vm["file-name"].as<std::string>();

    if ( vm.count( "history-file-name" ) )
    {
        m_file_name = vm["history-file-name"].as<std::string>();
    }
    else
    {
        m_file_name  = boost::filesystem::change_extension( name, ".history" ).string();
    }

    if ( vm.count( "review-file-name" ) )
    {
        m_review_name = vm["review-file-name"].as<std::string>();
    }
    else
    {
        m_review_name  = boost::filesystem::change_extension( name, ".review" ).string();
    }

    if ( vm.count( "max-cache-size" ) )
    {
        m_max_cache_size = vm["max-cache-size"].as<size_t>();
    }

    m_log_debug.add_attribute( "Level", boost::log::attributes::constant<std::string>( "DEBUG" ) );
    m_log_trace.add_attribute( "Level", boost::log::attributes::constant<std::string>( "TRACE" ) );
    m_log_test.add_attribute( "Level", boost::log::attributes::constant<std::string>( "TEST" ) );

    {
        std::stringstream strm;
        boost::chrono::seconds s;
        std::vector<std::string> string_list;

        if ( vm.count( "review-time-span-list" ) )
        {
            string_list = vm["review-time-span-list"].as< std::vector<std::string> >();
        }
        else
        {
            const char* s[] =
            {
                "0 seconds",
                "7 minutes",    "30 minutes",   "30 minutes",   "30 minutes",   "1 hours",      "1 hours",      "1 hours",
                "1 hours",      "2 hours",      "3 hours",      "4 hours",      "5 hours",      "6 hours",      "7 hours",
                "8 hours",      "9 hours",      "10 hours",     "11 hours",     "12 hours",     "13 hours",     "14 hours",
                "24 hours",     "48 hours",     "72 hours",     "96 hours",     "120 hours",    "144 hours",    "168 hours",
                "192 hours",    "216 hours",    "240 hours",    "264 hours",    "288 hours",    "312 hours",    "336 hours"
            };

            string_list.assign( s, s + sizeof(s) / sizeof(char*) );
        }

        for ( size_t i = 0; i < string_list.size(); ++i )
        {
            strm.clear();
            strm.str( string_list[i] );
            strm >> s;
            m_review_spans.push_back( s.count() );
        }

        strm.clear();
        strm.str("");
        std::copy( string_list.begin(), string_list.end(), std::ostream_iterator<std::string>( strm, ", " ) );
        BOOST_LOG(m_log_trace) << __FUNCTION__ << " - review-time-span(" << string_list.size() << "): " << strm.str();
    }
}


void History::initialize()
{
    bool should_write_history = false;
    history_type history = load_history_from_file( m_file_name );

    merge_history( history );

    if ( m_history != history )
    {
        BOOST_LOG(m_log_debug) << __FUNCTION__ << " - wrong history detected.";
        should_write_history = true;
    }

    history_type review_history = load_history_from_file( m_review_name );

    if ( ! review_history.empty() )
    {
        BOOST_LOG(m_log_debug) << __FUNCTION__ << " - review detected.";
        merge_history( review_history );
        boost::filesystem::remove( m_review_name );
        should_write_history = true;
    }

    if ( should_write_history )
    {
        write_history();
    }

    BOOST_LOG(m_log_debug) << __FUNCTION__ << " - history is uptodate.\n" << Utility::get_history_string( m_history );
}


std::time_t History::get_last_review_time( size_t hash )
{
    const time_list& t = m_history[hash];
    return t.empty() ? 0 : t.back();
}


size_t History::get_review_round( size_t hash )
{
    return m_history[hash].size();
}


void History::save_history( size_t hash )
{
    std::time_t current_time = std::time(0);

    m_history[hash].push_back( current_time );

    if ( m_cache_size == 0 )
    {
        m_review_stream.open( m_review_name.c_str(), std::ios::app );
    }

    if ( ! m_review_stream )
    {
        BOOST_LOG(m_log) << __FUNCTION__ << " - cannot open for append: " << m_review_name;
        return;
    }

    m_review_stream << hash << "\t" << current_time << std::endl;

    if ( m_review_stream.fail() )
    {
        BOOST_LOG(m_log) << __FUNCTION__ << " - failed.";
    }

    m_cache_size++;

    if ( m_max_cache_size <= m_cache_size )
    {
        write_history();
        clean_review_cache();
    }
}


void History::write_history()
{
    std::ofstream os( m_file_name.c_str() );

    if ( ! os )
    {
        BOOST_LOG(m_log) << __FUNCTION__ << " - can not open file for write " << m_file_name;
        return;
    }

    for ( history_type::const_iterator it = m_history.begin(); it != m_history.end(); ++it )
    {
        os << it->first << " ";
        std::copy( it->second.begin(), it->second.end(), std::ostream_iterator<std::time_t>(os, " ") );
        os << std::endl;
    }

    BOOST_LOG(m_log_debug) << __FUNCTION__ << " - " << "update history, size = " << m_history.size();
}


history_type History::load_history_from_file( const std::string& file_name )
{
    history_type history;

    if ( ! boost::filesystem::exists( file_name ) )
    {
        return history;
    }

    std::ifstream is( file_name.c_str() );

    if ( ! is )
    {
        return history;
    }

    size_t hash = 0;
    std::time_t time = 0;
    std::stringstream strm;

    for ( std::string s; std::getline( is, s ); )
    {
        if ( ! s.empty() )
        {
            strm.clear();
            strm.str(s);
            strm >> hash;
            std::vector<std::time_t>& times = history[hash];

            while ( strm >> time )
            {
                times.push_back( time );
            }
        }
    }

    return history;
}


void History::merge_history( const history_type& history )
{
    for ( history_type::const_iterator it = history.begin(); it != history.end(); ++it )
    {
        size_t hash = it->first;
        const std::vector<std::time_t>& times = it->second;
        std::vector<std::time_t>& history_times = m_history[hash];
        size_t round = history_times.size();
        std::time_t last_time = ( round ? history_times.back() : 0 );

        for ( size_t i = 0; i < times.size(); ++i )
        {
            if ( last_time + m_review_spans[round] < times[i] )
            {
                history_times.push_back( times[i] );
                last_time = times[i];
                round++;
            }
        }
    }
}


void History::synchronize_history( const std::set<size_t>& hashes )
{
    bool history_changed = false;

    for ( history_type::iterator it = m_history.begin(); it != m_history.end(); NULL )
    {
        if ( hashes.find( it->first ) == hashes.end() )
        {
            BOOST_LOG(m_log_debug) << __FUNCTION__ << " - " << "erase: " << it->first << " " << Utility::get_time_list_string( it->second );
            m_history.erase( it++ );
            history_changed = true;
        }
        else
        {
            ++it;
        }
    }

    for ( std::set<size_t>::const_iterator it = hashes.begin(); it != hashes.end(); ++it )
    {
        if ( m_history.find( *it ) == m_history.end() )
        {
            m_history.insert( history_type::value_type( *it, time_list() ) );
            history_changed = true;
            BOOST_LOG(m_log_debug) << __FUNCTION__ << " - " << "add: " << *it;
        }
    }

    if ( history_changed )
    {
        write_history();
        clean_review_cache();
    }
}


bool History::is_expired( size_t hash )
{
    time_list& times = m_history[hash];
    size_t review_round = times.size();

    if ( 0 == review_round )
    {
        return true;
    }

    if ( m_review_spans.size() == review_round )
    {
        BOOST_LOG(m_log_debug) << __FUNCTION__ << " - finished (" << hash << ")";
        return false;
    }

    std::time_t current_time = std::time(0);
    std::time_t last_review_time = times.back();
    std::time_t span = m_review_spans[review_round];
    return ( last_review_time + span < current_time );
}


std::set<size_t> History::get_expired()
{
    std::set<size_t> expired;

    for ( history_type::iterator it = m_history.begin(); it != m_history.end(); ++it )
    {
        if ( is_expired( it->first ) )
        {
            expired.insert( it->first );
        }
    }

    return expired;
}


void History::clean_review_cache()
{
    if ( m_review_stream.is_open() )
    {
        m_review_stream.close();
    }

    if ( boost::filesystem::exists( m_review_name ) )
    {
        boost::filesystem::remove( m_review_name );
    }

    m_cache_size = 0;
}
