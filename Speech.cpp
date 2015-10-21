#include "stdafx.h"
#include "Speech.h"
#include "Utility.h"


extern boost::log::sources::logger m_log_debug;


Speech::Speech( const boost::program_options::variables_map& vm )
    : m_variables_map( vm )
{
    if ( vm.count( "speech-path" ) )
    {
        std::vector<std::string> vs = vm["speech-path"].as< std::vector<std::string> >();

        for ( size_t i = 0; i < vs.size(); ++i )
        {
            m_path.push_back( vs[i] );
        }
    }
}


void Speech::play( const std::string& word )
{
    if ( word.empty() )
    {
        return;
    }

    std::string first_char = word.substr( 0, 1 );

    for ( size_t i = 0; i < m_path.size(); ++i )
    {
        boost::filesystem::path p = m_path[i] / first_char / ( word + ".mp3" );

        if ( boost::filesystem::exists( p ) )
        {
            Utility::play_sound_thread( p.string() );
        }
    }
}


void Speech::play( const std::vector<std::string>& words )
{
    std::vector<std::string> pahts;

    for ( size_t j = 0; j < m_path.size(); ++j )
    {
        for ( size_t i = 0; i < words.size(); ++i )
        {
            if ( words[i].empty() )
            {
                continue;
            }

            std::string first_char = words[i].substr( 0, 1 );

            boost::filesystem::path p = m_path[j] / first_char / ( words[i] + ".mp3" );

            if ( boost::filesystem::exists( p ) )
            {
                pahts.push_back( p.string() );
                BOOST_LOG(m_log_debug) << __FUNCTION__ << " - speech: " << words[i] << " " << p.string() ;
            }
            else
            {
                BOOST_LOG(m_log_debug) << __FUNCTION__ << " - speech: " << words[i];
            }
        }
    }

    Utility::play_sound_thread( pahts );
}
