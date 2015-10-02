#include "stdafx.h"
#include "SingleLineReview.h"


SingleLineReview::SingleLineReview( const std::string& file_name )
    : m_file_name( file_name ),
      m_last_modified_time( 0 )
{
    m_review_name   = boost::filesystem::change_extension( m_file_name, ".review" ).string();
    m_history_name  = boost::filesystem::change_extension( m_file_name, ".history" ).string();

    const size_t span[] = { 0, 60, 3*60, 7*60, 15*60, 30*60, 60*60, 3*60*60, 7*60*60, 24*60*60, 3*24*60*60, 7*24*60*60, 15*24*60*60, 30*24*60*60, 3*30*24*60*60 };
    m_review_span.assign( span, span + sizeof(span) / sizeof(size_t) );
}


void SingleLineReview::review()
{
    reload_history();

    for ( ;; )
    {
        reload_strings();
        synchronize_history();
        std::vector< std::pair<std::string, size_t> > strings = collect_strings_to_review();

        system( "CLS" );
        std::cout << "\t****** there are " << strings.size() << " items to review ******" << std::endl;
        wait_for_input();

        if ( ! strings.empty() )
        {
            m_review_strm.open( m_review_name.c_str(), std::ios::app );

            std::random_shuffle( strings.begin(), strings.end() );

            for ( size_t i = 0; i < strings.size(); ++i )
            {
                display_string_to_review( strings, i );
                wait_for_input();
                save_review_time( strings[i] );
            }

            m_review_strm.close();
        }

        save_review_to_history();
    }
}


bool SingleLineReview::reload_history()
{
    try
    {
        if ( ! boost::filesystem::exists( m_history_name ) )
        {
            return true;
        }
    }
    catch ( boost::filesystem::filesystem_error& e )
    {
        BOOST_LOG(m_log) << __FUNCTION__ << " - " << e.what();
        throw;
    }

    std::ifstream is( m_history_name.c_str() );

    if ( ! is )
    {
        BOOST_LOG(m_log) << __FUNCTION__ << " - can not open file " << m_history_name;
        return false;
    }

    m_history.clear();

    for ( std::string s; std::getline( is, s ); )
    {
        if ( ! s.empty() )
        {
            size_t hash = 0;
            std::time_t time = 0;
            std::stringstream strm( s );

            strm >> hash;

            while ( strm >> time )
            {
                m_history[hash].push_back(time);
            }
        }
    }

    return true;
}


bool SingleLineReview::reload_strings()
{
    if ( ! check_last_write_time() )
    {
        return true;
    }

    std::ifstream is( m_file_name.c_str() );

    if ( !is )
    {
        BOOST_LOG(m_log) << __FUNCTION__ << " - cannot open file: " << m_file_name;
        return false;
    }

    m_strings.clear();

    for ( std::string s; std::getline( is, s ); )
    {
        boost::trim(s);

        if ( ! s.empty() )
        {
            m_strings.push_back( std::make_pair( s, m_string_hash(s) ) );
        }
    }

    return true;
}


bool SingleLineReview::check_last_write_time()
{
    try
    {
        std::time_t last_write_time = boost::filesystem::last_write_time( m_file_name );

        if ( m_last_modified_time < last_write_time )
        {
            m_last_modified_time = last_write_time;
            return true;
        }
    }
    catch ( boost::filesystem::filesystem_error& e )
    {
    	BOOST_LOG(m_log) << __FUNCTION__ << " - " << e.what();
        throw;
    }

    return false;
}


std::vector< std::pair<std::string, size_t> > SingleLineReview::collect_strings_to_review()
{
    std::vector< std::pair<std::string, size_t> > strings;
    
    for ( size_t i = 0; i < m_strings.size(); ++i )
    {
        size_t hash = m_strings[i].second;
        std::vector<std::time_t>& times = m_history[hash];

        if ( times.empty() )
        {
            strings.push_back( m_strings[i] );
        }
        else
        {
            size_t review_rounds = times.size();

            if ( m_review_span.size() < review_rounds )
            {
                continue;
            }

            std::time_t last_review_time = m_history[hash].back();
            std::time_t now = std::time(0);

            if ( last_review_time + m_review_span[review_rounds] < now )
            {
                strings.push_back( m_strings[i] );
            }
        }
    }

    return strings;
}


void SingleLineReview::save_review_time( const std::pair<std::string, size_t>& s )
{
    if ( ! m_review_strm )
    {
        BOOST_LOG(m_log) << __FUNCTION__ << " - cannot open for append: " << m_review_name;
        return;
    }

    m_review_strm << s.second << "\t" << std::time(0) << std::endl;

    if ( m_review_strm.fail() )
    {
        BOOST_LOG(m_log) << __FUNCTION__ << " - failed.";
    }
}


void SingleLineReview::wait_for_input()
{
    std::string line;
    std::getline( std::cin, line );
    system( "CLS" );
}


void SingleLineReview::save_review_to_history()
{
    bool BOOST_LOCAL_FUNCTION( const bind& m_review_name, bind& m_history, const bind& m_review_span, bind& m_log )
    {
        if ( ! boost::filesystem::exists( m_review_name ) )
        {
            return true;
        }

        std::ifstream is( m_review_name.c_str() );

        if ( ! is )
        {
            BOOST_LOG(m_log) << __FUNCTION__ << " - can not open file " << m_review_name;
            return false;
        }

        for ( std::string s; std::getline( is, s ); )
        {
            if ( ! s.empty() )
            {
                size_t hash = 0;
                std::time_t time = 0;
                std::time_t last_time = 0;
                std::stringstream strm( s );

                strm >> hash >> time;

                std::vector<std::time_t>& times = m_history[hash];

                if ( ! times.empty() )
                {
                    last_time = times.back();
                }

                if ( last_time + m_review_span[times.size()] < time )
                {
                    times.push_back( time );
                }
            }
        }

        return true;
    }
    BOOST_LOCAL_FUNCTION_NAME( load_review_and_merge_to_history );

    bool BOOST_LOCAL_FUNCTION( const bind& m_history_name, const bind& m_history, bind& m_log )
    {
        std::ofstream os( m_history_name.c_str() );

        if ( ! os )
        {
            BOOST_LOG(m_log) << __FUNCTION__ << " - can not open file for write " << m_history_name;
            return false;
        }

        for ( std::map< size_t, std::vector<std::time_t> >::const_iterator it = m_history.begin(); it != m_history.end(); ++it )
        {
            os << it->first << " ";
            std::copy( it->second.begin(), it->second.end(), std::ostream_iterator<std::time_t>(os, " ") );
            os << std::endl;
        }

        return true;
    }
    BOOST_LOCAL_FUNCTION_NAME( overwrite_history );

    if ( ! boost::filesystem::exists( m_review_name ) )
    {
        return;
    }

    try
    {
        if ( load_review_and_merge_to_history() && overwrite_history() )
        {
            boost::filesystem::remove( m_review_name );
        }
    }
    catch ( boost::filesystem::filesystem_error& e )
    {
    	BOOST_LOG(m_log) << __FUNCTION__ << " - " << e.what();
        throw;
    }
}


void SingleLineReview::synchronize_history()
{
    std::set<size_t> hashes;

    for ( size_t i = 0; i < m_strings.size(); ++i )
    {
        hashes.insert( m_strings[i].second );
    }

    for ( std::map< size_t, std::vector<std::time_t> >::iterator it = m_history.begin(); it != m_history.end(); NULL )
    {
        if ( hashes.find( it->first ) == hashes.end() )
        {
            m_history.erase( it++ );
        }
        else
        {
            ++it;
        }
    }
}


void SingleLineReview::display_string_to_review( const std::vector< std::pair<std::string, size_t> >& strings, size_t index )
{
    std::cout << "\t" << strings[index].first << " [" << index + 1 << "/" << strings.size() << "] " << std::flush;
}
