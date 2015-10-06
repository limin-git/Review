#include "stdafx.h"
#include "SingleLineReview.h"


SingleLineReview::SingleLineReview( const std::string& file_name, const boost::program_options::variables_map& vm )
    : m_file_name( file_name ),
      m_last_modified_time( 0 ),
      m_variable_map( vm ),
      m_is_reviewing( false )
{
    m_review_name   = boost::filesystem::change_extension( m_file_name, ".review" ).string();
    m_history_name  = boost::filesystem::change_extension( m_file_name, ".history" ).string();
    enum { minute = 60, hour = 60 * minute ,day = 24 * minute, month = 30 * day };
    const size_t span[] =
    {
        0*minute,   1*minute,   3*minute,   7*minute,   15*minute,  30*minute,  30*minute,
        1*hour,     3*hour,     7*hour,     7*hour,     7*hour,     7*hour,     7*hour,
        1*day,      2*day,      3*day,      4*day,      5*day,      6*day,      7*day,
        7*day,      7*day,      7*day,      7*day,      7*day,      7*day,      7*day,
        1*month,    1*month,    1*month,    1*month,    1*month,    1*month,    1*month
    };
    m_review_span.assign( span, span + sizeof(span) / sizeof(size_t) );
    new boost::thread( boost::bind( &SingleLineReview::collect_strings_to_review_thread, this ) );
}


void SingleLineReview::review()
{
    reload_history();

    for ( ;; )
    {
        reload_strings();
        synchronize_history();
        collect_strings_to_review();
        wait_for_input( "\t****** there are " + boost::lexical_cast<std::string>(m_reviewing_strings.size() ) + " items to review ******\n" );

        if ( ! m_reviewing_strings.empty() )
        {
            on_review_begin();

            for ( size_t i = 0; i < m_reviewing_strings.size(); ++i )
            {
                display_reviewing_string( m_reviewing_strings, i );
                wait_for_input();
                save_review_time( m_reviewing_strings[i] );
            }

            on_review_end();
        }

        save_review_to_history();
    }
}


void SingleLineReview::on_review_begin()
{
    m_is_reviewing = true;
    m_review_strm.open( m_review_name.c_str(), std::ios::app );
    std::random_shuffle( m_reviewing_strings.begin(), m_reviewing_strings.end() );
    system( ( "TITLE " + m_file_name + " - " + boost::lexical_cast<std::string>( m_reviewing_strings.size() ) ).c_str() );
}


void SingleLineReview::on_review_end()
{
    m_is_reviewing = false;
    m_review_strm.close();
    m_reviewing_strings.clear();
    system( ( "TITLE " + m_file_name ).c_str() );
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


void SingleLineReview::collect_strings_to_review()
{
    boost::unique_lock<boost::mutex> guard( m_mutex );

    m_reviewing_strings.clear();
    
    for ( size_t i = 0; i < m_strings.size(); ++i )
    {
        size_t hash = m_strings[i].second;
        std::vector<std::time_t>& times = m_history[hash];

        if ( times.empty() )
        {
            m_reviewing_strings.push_back( m_strings[i] );
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
                m_reviewing_strings.push_back( m_strings[i] );
            }
        }
    }
}


void SingleLineReview::collect_strings_to_review_thread()
{
    void BOOST_LOCAL_FUNCTION( const bind& m_file_name, const bind& m_reviewing_strings )
    {
        system( ( "TITLE " + m_file_name + " - " + boost::lexical_cast<std::string>( m_reviewing_strings.size() ) ).c_str() ); 
    }
    BOOST_LOCAL_FUNCTION_NAME( set_title );

    while ( true )
    {
        boost::this_thread::sleep_for( boost::chrono::seconds(30) );

        if ( false == m_is_reviewing )
        {
            collect_strings_to_review();
            set_title();
        }
    }
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


void SingleLineReview::wait_for_input( const std::string& message )
{
    if ( ! message.empty() )
    {
        std::cout << message << std::flush;
    }

    std::getline( std::cin, std::string() );
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


void SingleLineReview::display_reviewing_string( const std::vector< std::pair<std::string, size_t> >& strings, size_t index )
{
    const std::string& s = strings[index].first;

    if ( boost::starts_with( s, "[Q]" ) )
    {
        size_t pos = s.find( "[A] " );

        if ( pos != std::string::npos )
        {
            std::string question = s.substr( 4, pos - 4 );
            std::string answer = s.substr( pos + 4 );
            boost::trim(question);
            boost::trim(answer);
            std::cout << "\t" << question << std::flush;
            std::getline( std::cin, std::string() );
            std::cout << "\t" << answer << " [" << index + 1 << "/" << strings.size() << "] " << std::flush;
        }
        else
        {
            BOOST_LOG(m_log) << __FUNCTION__ << " - bad format: " << s;
        }
    }
    else
    {
        std::cout << "\t" << strings[index].first << " [" << index + 1 << "/" << strings.size() << "] " << std::flush;
    }
}
