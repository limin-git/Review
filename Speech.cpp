#include "stdafx.h"
#include "Speech.h"
#include "Utility.h"


Speech::Speech( const boost::program_options::variables_map& vm )
    : m_variables_map( vm )
{
    if ( vm.count( "speech-path" ) )
    {
        m_path = vm["speech-path"].as<std::string>();
    }
}


void Speech::play( const std::string& word )
{
    if ( word.empty() )
    {
        return;
    }

    std::string first_char = word.substr( 0, 1 );
    boost::filesystem::path p = m_path / first_char / ( word + ".mp3" );
    
    if ( boost::filesystem::exists( p ) )
    {
        Utility::play_sound_thread( p.string() );
    }
}


void Speech::play( const std::vector<std::string>& words )
{
    if ( words.empty() )
    {
        return;
    }

    std::vector<std::string> pahts;

    for ( size_t i = 0; i < words.size(); ++i )
    {
        std::string first_char = words[i].substr( 0, 1 );
        boost::filesystem::path p = m_path / first_char / ( words[i] + ".mp3" );

        if ( boost::filesystem::exists( p ) )
        {
            pahts.push_back( p.string() );
        }
    }

    Utility::play_sound_thread( pahts );
}
