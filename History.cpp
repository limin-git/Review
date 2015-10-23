#include "stdafx.h"
#include "History.h"
#include "Utility.h"
#include "Log.h"
#include "OptionString.h"


History::History( const boost::program_options::variables_map& vm )
    : m_variables_map( vm ),
      m_max_cache_size( 100 ),
      m_cache_size( 0 )
{
    std::string name = vm[file_name_option].as<std::string>();

    if ( vm.count( file_history_option ) )
    {
        m_file_name = vm[file_history_option].as<std::string>();
    }
    else
    {
        m_file_name  = boost::filesystem::change_extension( name, ".history" ).string();
    }

    if ( vm.count( file_review_option ) )
    {
        m_review_name = vm[file_review_option].as<std::string>();
    }
    else
    {
        m_review_name  = boost::filesystem::change_extension( name, ".review" ).string();
    }

    if ( vm.count( review_max_cache_size_option ) )
    {
        m_max_cache_size = vm[review_max_cache_size_option].as<size_t>();
    }

    {
        std::stringstream strm;
        boost::chrono::seconds s;
        std::vector<std::string> string_list;

        if ( vm.count( review_time_span_list_option ) )
        {
            string_list = vm[review_time_span_list_option].as< std::vector<std::string> >();
        }
        else
        {
            const char* s[] =
            {
                "0 seconds",    "7 minutes",    "30 minutes",   "30 minutes",   "1 hours",      "3 hours",      "5 hours",
                "7 hours",      "9 hours",      "11 hours",     "13 hours",     "15 hours",     "17 hours",     "19 hours",
                "24 hours",     "48 hours",     "72 hours",     "96 hours",     "120 hours",    "144 hours",    "168 hours",
                "192 hours",    "216 hours",    "240 hours",    "264 hours",    "288 hours",    "312 hours",    "336 hours",
                "360 hours",    "384 hours",    "408 hours",    "432 hours",    "456 hours",    "480 hours",    "504 hours",
                "528 hours",    "552 hours",    "576 hours",    "600 hours",    "624 hours",    "648 hours",    "672 hours",
                "696 hours",    "720 hours",    "744 hours",    "768 hours",    "792 hours",    "816 hours",    "840 hours"
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
        LOG_TRACE << "review-time-span(" << string_list.size() << "): " << strm.str();
    }
}


void History::initialize()
{
    bool should_write_history = false;
    history_type history = load_history_from_file( m_file_name );

    merge_history( history );

    if ( m_history != history )
    {
        LOG_DEBUG << "wrong history detected.";
        should_write_history = true;
    }

    history_type review_history = load_history_from_file( m_review_name );

    if ( ! review_history.empty() )
    {
        LOG_DEBUG << "review detected.";
        merge_history( review_history );
        boost::filesystem::remove( m_review_name );
        should_write_history = true;
    }

    if ( should_write_history )
    {
        write_history();
    }

    LOG_TRACE << "history is updated.\n" << Utility::get_history_string( m_history );
}


void History::save_history( size_t hash )
{
    std::time_t current_time = std::time(0);

    m_history[hash].push_back( current_time );

    if ( m_cache_size == 0 )
    {
        m_review_stream.open( m_review_name.c_str(), std::ios::app );

        if ( ! m_review_stream )
        {
            LOG << "cannot open for append: " << m_review_name;
            return;
        }

        LOG_DEBUG << "created a file for cache: " << m_review_name;
    }


    m_review_stream << hash << "\t" << current_time << std::endl;

    if ( m_review_stream.fail() )
    {
        LOG << "failed.";
    }

    m_cache_size++;
    LOG_DEBUG << "cache-size = " << m_cache_size;

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
        LOG << "can not open file for write " << m_file_name;
        return;
    }

    for ( history_type::const_iterator it = m_history.begin(); it != m_history.end(); ++it )
    {
        os << it->first << " ";
        std::copy( it->second.begin(), it->second.end(), std::ostream_iterator<std::time_t>(os, " ") );
        os << std::endl;
    }

    LOG_DEBUG << "update history, size = " << m_history.size();
}


history_type History::load_history_from_file( const std::string& file )
{
    history_type history;

    if ( ! boost::filesystem::exists( file ) )
    {
        return history;
    }

    std::ifstream is( file.c_str() );

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

    LOG_TRACE << file << std::endl << Utility::get_history_string( history );
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
            else
            {
                LOG_DEBUG
                    << " ignore review time: " << Utility::time_string( times[i] )
                    << " round = " << round
                    << " last-review-time = " << last_time
                    << " elapsed = " << Utility::time_duration_string( times[i] - last_time )
                    << " span = " << Utility::time_duration_string( m_review_spans[round] )
                    ;
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
            LOG_DEBUG << "erase: " << it->first << " " << Utility::get_time_list_string( it->second );
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
            LOG_DEBUG << "add: " << *it;
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
        LOG_DEBUG << "finished (" << hash << ")";
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
        LOG_DEBUG << "remove file: " << m_review_name;
    }

    m_cache_size = 0;
}
