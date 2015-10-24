#include "stdafx.h"
#include "Speech.h"
#include "Utility.h"
#include "Log.h"
#include "OptionString.h"
#include "ConfigFileMonitor.h"


Speech::Speech( const boost::program_options::variables_map& vm )
    : m_variables_map( vm )
{
    update_option( vm );
    m_connection = ConfigFileMonitor::connect_to_signal( boost::bind( &Speech::update_option, this, _1 ) );
}


Speech::~Speech()
{
    m_connection.disconnect();
}


void Speech::play( const std::string& word )
{
    std::vector<std::string> files = get_files( word );

    if ( ! files.empty() )
    {
        Utility::play_sound_thread( files );
    }
}


void Speech::play( const std::vector<std::string>& words )
{
    std::vector<std::string> files = get_files( words );

    if ( ! files.empty() )
    {
        Utility::play_sound_thread( files );
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
            LOG_TRACE << wav.string();
        }
        else if ( boost::filesystem::exists( mp3 ) )
        {
            files.push_back( mp3.string() );
            LOG_TRACE << mp3.string();
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
                LOG_TRACE << wav.string();
            }
            else if ( boost::filesystem::exists( mp3 ) )
            {
                files.push_back( mp3.string() );
                LOG_TRACE << mp3.string();
            }
        }
    }

    return files;
}


void Speech::update_option( const boost::program_options::variables_map& vm )
{
    if ( vm.count( speech_path_option ) )
    {
        std::vector<std::string> vs = vm[speech_path_option].as< std::vector<std::string> >();

        if ( m_speech_path_option == vs )
        {
            return;
        }

        m_speech_path_option = vs;
        std::vector<boost::filesystem::path> paths;

        for ( size_t i = 0; i < vs.size(); ++i )
        {
            if ( boost::filesystem::exists( vs[i] ) )
            {
                paths.push_back( vs[i] );
                LOG_DEBUG << "speech path: " << vs[i];
            }
            else
            {
                LOG_DEBUG << "invalide path: " << vs[i];
            }
        }

        m_paths = paths;
    }
}
