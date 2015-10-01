#include "stdafx.h"
#include "SingleLineReview.h"


SingleLineReview::SingleLineReview( const std::string& file_name )
    : m_file_name( file_name )
{
    m_review_name   = boost::filesystem::change_extension( m_file_name, ".review" ).string();
    m_history_name  = boost::filesystem::change_extension( m_file_name, ".history" ).string();
    m_ignore_name   = boost::filesystem::change_extension( m_file_name, ".ignore" ).string();
}


bool SingleLineReview::initialize()
{
    return
        load_strings() &&
        load_history();
}


bool SingleLineReview::load_strings()
{
    std::ifstream is( m_file_name.c_str() );

    if ( !is )
    {
        BOOST_LOG(m_log) << __FUNCTION__ << " - cannot open file: " << m_file_name;
        return false;
    }

    boost::hash<std::string> string_hash;

    for ( std::string s; std::getline( is, s ); )
    {
        boost::trim(s);

        if ( ! s.empty() )
        {
            m_strings.push_back( std::make_pair( s, string_hash(s) ) );
        }
    }

    return true;
}


bool SingleLineReview::load_history()
{
    if ( ! boost::filesystem::exists( m_history_name ) )
    {
        return true;
    }

    std::ifstream is( m_history_name.c_str() );

    if ( ! is )
    {
        BOOST_LOG(m_log) << __FUNCTION__ << " - can not open file " << m_history_name;
        return false;
    }

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


std::vector< std::pair<std::string, size_t> > SingleLineReview::collect_strings_to_review()
{
    std::vector< std::pair<std::string, size_t> > strings;
    const size_t gaps[] = { 0, 60, 7*60, 30*60, 60*60, 3*60*60, 24*60*60, 3*24*60*60, 7*24*60*60, 15*24*60*60, 30*24*60*60, 3*30*24*60*60 };
    
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
            std::time_t last_review_time = m_history[hash].back();
            std::time_t now = std::time(0);

            if ( last_review_time + gaps[review_rounds] < now )
            {
                strings.push_back( m_strings[i] );
            }
        }
    }

    return strings;
}


void SingleLineReview::save_review_time( const std::pair<std::string, size_t>& s )
{
    m_review_strm << s.second << "\t" << std::time(0) << std::endl;

    if ( m_review_strm.fail() )
    {
        BOOST_LOG(m_log) << __FUNCTION__ << " - failed.";
    }
}


void SingleLineReview::review()
{
    std::string line;

    for ( size_t round = 1; true ; ++round )
    {
        m_review_strm.open( m_review_name.c_str(), std::ios::app );

        if ( ! m_review_strm )
        {
            BOOST_LOG(m_log) << __FUNCTION__ << " - cannot open for append: " << m_review_name;
            return;
        }

        std::vector< std::pair<std::string, size_t> > strings = collect_strings_to_review();

        std::cout << "****** there are " << strings.size() << " items to review ******" << std::endl;
        std::getline( std::cin, line );
        system( "CLS" );

        std::random_shuffle( strings.begin(), strings.end() );

        for ( size_t i = 0; i < strings.size(); ++i )
        {
            std::cout << "\t" << strings[i].first << " [" << round << ":" << i + 1 << "/" << strings.size() << "] " << std::flush;
            save_review_time( strings[i] );
            std::getline( std::cin, line );
            std::cout << std::endl;
        }

        m_review_strm.close();
        save_review_to_history();
    }
}


void SingleLineReview::save_review_to_history()
{
    std::map< size_t, std::vector<std::time_t> >& hash_2_time_map = m_history;

    bool BOOST_LOCAL_FUNCTION( bind this_, bind& hash_2_time_map )
    {
        if ( ! boost::filesystem::exists( this_->m_history_name ) )
        {
            return true;
        }

        std::ifstream is( this_->m_history_name.c_str() );

        if ( ! is )
        {
            BOOST_LOG(this_->m_log) << __FUNCTION__ << " - can not open file " << this_->m_history_name;
            return false;
        }

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
                    hash_2_time_map[hash].push_back(time);
                }
            }
        }

        return true;
    }
    BOOST_LOCAL_FUNCTION_NAME( load_history );

    bool BOOST_LOCAL_FUNCTION( bind this_, bind& hash_2_time_map )
    {
        const size_t gaps[] = { 0, 60, 7*60, 30*60, 60*60, 3*60*60, 24*60*60, 3*24*60*60, 7*24*60*60, 15*24*60*60, 30*24*60*60, 3*30*24*60*60 };

        if ( ! boost::filesystem::exists( this_->m_review_name ) )
        {
            return false;
        }

        std::ifstream is( this_->m_review_name.c_str() );

        if ( ! is )
        {
            BOOST_LOG(this_->m_log) << __FUNCTION__ << " - can not open file " << this_->m_review_name;
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

                if ( ! hash_2_time_map[hash].empty() )
                {
                    last_time = hash_2_time_map[hash].back();
                }

                if ( last_time + gaps[ hash_2_time_map[hash].size() ] < time )
                {
                    hash_2_time_map[hash].push_back( time );
                }
            }
        }

        return true;
    }
    BOOST_LOCAL_FUNCTION_NAME( load_review_and_merge_to_history );

    bool BOOST_LOCAL_FUNCTION( bind this_, bind& hash_2_time_map )
    {
        std::ofstream os( this_->m_history_name.c_str() );

        if ( ! os )
        {
            BOOST_LOG(this_->m_log) << __FUNCTION__ << " - can not open file for write " << this_->m_history_name;
            return false;
        }

        for ( std::map< size_t, std::vector<std::time_t> >::iterator it = hash_2_time_map.begin(); it != hash_2_time_map.end(); ++it )
        {
            os << it->first << " ";
            std::copy( it->second.begin(), it->second.end(), std::ostream_iterator<std::time_t>(os, " ") );
            os << std::endl;
        }

        return true;
    }
    BOOST_LOCAL_FUNCTION_NAME( overwrite_history );

    if ( load_history() )
    {
        if ( load_review_and_merge_to_history() )
        {
            if ( overwrite_history() )
            {
                if ( boost::filesystem::exists( m_review_name ) )
                {
                    boost::system::error_code ec;
                    boost::filesystem::remove( m_review_name, ec );

                    if ( ec )
                    {
                        BOOST_LOG(m_log) << __FUNCTION__ << " - failed to remove file " << m_review_name << ", " << ec.message();
                    }
                }
            }
        }
    }
}
