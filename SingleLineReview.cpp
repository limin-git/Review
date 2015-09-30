#include "stdafx.h"
#include "SingleLineReview.h"
#include "Library/Utility.h"
#include <boost/local_function.hpp>


SingleLineReview::SingleLineReview( const std::string& file_name )
    : m_file_name( file_name )
{
    m_review_name   = boost::filesystem::change_extension( m_file_name, ".review" ).string();
    m_history_name  = boost::filesystem::change_extension( m_file_name, ".history" ).string();
    m_ignore_name   = boost::filesystem::change_extension( m_file_name, ".ignore" ).string();
}


bool SingleLineReview::initialize()
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
        if ( ! s.empty() )
        {
            m_strings.push_back( std::make_pair( s, string_hash(s) ) );
        }
    }

    return true;
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
    m_review_strm.open( m_review_name.c_str(), std::ios::app );

    if ( ! m_review_strm )
    {
        BOOST_LOG(m_log) << __FUNCTION__ << " - cannot open for append: " << m_review_name;
        return;
    }

    for ( size_t round = 1; true ; ++round )
    {
        std::cout << "****** there are " << m_strings.size() << " items to review ******" << std::endl;
        system( "PAUSE > NUL" );
        system( "CLS" );

        std::random_shuffle( m_strings.begin(), m_strings.end() );

        for ( size_t i = 0; i < m_strings.size(); ++i )
        {
            std::cout << "\t" << m_strings[i].first << " [" << round << ":" << i + 1 << "/" << m_strings.size() << "] " << std::flush;
            save_review_time( m_strings[i] );
            system( "PAUSE > NUL" );
            std::cout << std::endl;
        }
    }
}


void SingleLineReview::save_review_to_history( size_t minimal_gap_seconds )
{
    std::map< size_t, std::vector<std::time_t> > hash_2_time_map;

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

    bool BOOST_LOCAL_FUNCTION( bind this_, bind& hash_2_time_map, const bind minimal_gap_seconds )
    {
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

        bool is_changed = false;

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

                if ( minimal_gap_seconds < time - last_time )
                {
                    hash_2_time_map[hash].push_back( time );
                    is_changed = true;
                }
            }
        }

        return is_changed;
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
