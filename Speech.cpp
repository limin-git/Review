#include "stdafx.h"
#include "Speech.h"
#include "Utility.h"
#include "Log.h"
#include "OptionString.h"
#include "ProgramOptions.h"


Speech::Speech()
    : m_no_duplicate( false )
{
    ProgramOptions::connect_to_signal( boost::bind( &Speech::update_option, this, _1 ) );
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
    boost::unique_lock<boost::mutex> lock( m_mutex );

    std::vector<std::string>  files;
    std::string first_char = word.substr( 0, 1 );
    bool word_found = false;

    for ( size_t i = 0; i < m_paths.size(); ++i )
    {
        boost::filesystem::path mp3 = m_paths[i] / first_char / ( word + ".mp3" );
        boost::filesystem::path wav = m_paths[i] / first_char / ( word + ".wav" );

        if ( boost::filesystem::exists( wav ) )
        {
            files.push_back( wav.string() );
            word_found = true;
            LOG_TRACE << wav.string();
        }
        else if ( boost::filesystem::exists( mp3 ) )
        {
            files.push_back( mp3.string() );
            word_found = true;
            LOG_TRACE << mp3.string();
        }

        if ( m_no_duplicate && word_found )
        {
            break;
        }
    }

    return files;
}


std::vector<std::string> Speech::get_files( const std::vector<std::string>& words )
{
    boost::unique_lock<boost::mutex> lock( m_mutex );

    std::vector<std::string> files;
    std::vector<bool> word_found( words.size() );
    for ( size_t i = 0; i < word_found.size(); ++i ) { word_found[i] = false; }

    for ( size_t i = 0; i < m_paths.size(); ++i )
    {
        for ( size_t j = 0; j < words.size(); ++j )
        {
            if ( m_no_duplicate && word_found[j] == true )
            {
                continue;
            }

            std::string first_char = words[j].substr( 0, 1 );

            boost::filesystem::path mp3 = m_paths[i] / first_char / ( words[j] + ".mp3" );
            boost::filesystem::path wav = m_paths[i] / first_char / ( words[j] + ".wav" );

            if ( boost::filesystem::exists( wav ) )
            {
                files.push_back( wav.string() );
                word_found[j] = true;
                LOG_TRACE << wav.string();
            }
            else if ( boost::filesystem::exists( mp3 ) )
            {
                files.push_back( mp3.string() );
                word_found[j] = true;
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

        if ( m_speech_path_option != vs )
        {
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

            boost::unique_lock<boost::mutex> lock( m_mutex );
            m_paths = paths;
        }
    }

    if ( vm.count( speech_no_duplicate ) )
    {
        bool new_value = ( "true" == vm[speech_no_duplicate].as<std::string>() );

        if ( m_no_duplicate != new_value )
        {
            m_no_duplicate = new_value;
            LOG_DEBUG << "no-duplicate: " << m_no_duplicate;
        }
    }
}
