#include "stdafx.h"
#include "Speech.h"
#include "Utility.h"
#include "Log.h"


Speech::Speech( const boost::program_options::variables_map& vm )
    : m_variables_map( vm )
{
    if ( vm.count( "speech-path" ) )
    {
        std::vector<std::string> vs = vm["speech-path"].as< std::vector<std::string> >();

        for ( size_t i = 0; i < vs.size(); ++i )
        {
            m_paths.push_back( vs[i] );
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

    for ( size_t i = 0; i < m_paths.size(); ++i )
    {
        boost::filesystem::path mp3 = m_paths[i] / first_char / ( word + ".mp3" );
        boost::filesystem::path wav = m_paths[i] / first_char / ( word + ".wav" );

        if ( boost::filesystem::exists( wav ) )
        {
            Utility::play_sound_thread( wav.string() );
        }
        else if ( boost::filesystem::exists( mp3 ) )
        {
            Utility::play_sound_thread( mp3.string() );
        }
    }
}


void Speech::play( const std::vector<std::string>& words )
{
    std::vector<std::string> pahts;

    for ( size_t i = 0; i < m_paths.size(); ++i )
    {
        for ( size_t j = 0; j < words.size(); ++j )
        {
            if ( words[j].empty() )
            {
                continue;
            }

            std::string first_char = words[j].substr( 0, 1 );

            boost::filesystem::path mp3 = m_paths[i] / first_char / ( words[j] + ".mp3" );
            boost::filesystem::path wav = m_paths[i] / first_char / ( words[j] + ".wav" );

            if ( boost::filesystem::exists( wav ) )
            {
                pahts.push_back( wav.string() );
                LOG_DEBUG << "play: " << words[j] << " " << wav.string() ;
            }
            else if ( boost::filesystem::exists( mp3 ) )
            {
                pahts.push_back( mp3.string() );
                LOG_DEBUG << "play: " << words[j] << " " << mp3.string() ;
            }
            else
            {
                LOG_DEBUG << "play: " << words[j];
            }
        }
    }

    if ( ! pahts.empty() )
    {
        Utility::play_sound_thread( pahts );
    }
}


std::vector<std::string> Speech::get_files( const std::string& word )
{
    std::vector<std::string>  files;

    if ( word.empty() )
    {
        files;
    }

    std::string first_char = word.substr( 0, 1 );

    for ( size_t i = 0; i < m_paths.size(); ++i )
    {
        boost::filesystem::path mp3 = m_paths[i] / first_char / ( word + ".mp3" );
        boost::filesystem::path wav = m_paths[i] / first_char / ( word + ".wav" );

        if ( boost::filesystem::exists( wav ) )
        {
            files.push_back( wav.string() );
        }
        else if ( boost::filesystem::exists( mp3 ) )
        {
            files.push_back( mp3.string() );
        }
    }

    return files;
}


std::vector<std::string> Speech::get_files( const std::vector<std::string>& words )
{
    std::vector<std::string> files;

    for ( size_t i = 0; i < m_paths.size(); ++i )
    {
        for ( size_t j = 0; j < words.size(); ++j )
        {
            if ( words[j].empty() )
            {
                continue;
            }

            std::string first_char = words[j].substr( 0, 1 );

            boost::filesystem::path mp3 = m_paths[i] / first_char / ( words[j] + ".mp3" );
            boost::filesystem::path wav = m_paths[i] / first_char / ( words[j] + ".wav" );

            if ( boost::filesystem::exists( wav ) )
            {
                files.push_back( wav.string() );
            }
            else if ( boost::filesystem::exists( mp3 ) )
            {
                files.push_back( mp3.string() );
            }
        }
    }

    return files;
}
