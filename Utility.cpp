#include "stdafx.h"
#include "Utility.h"
#include "Log.h"
#include "QueueProcessor.h"
#include <dshow.h>
#pragma comment(lib, "strmiids.lib") // For IID_IGraphBuilder, IID_IMediaControl, IID_IMediaEvent


namespace Utility
{

    std::ostream& output_history( std::ostream& os, const history_type& history )
    {
        for ( history_type::const_iterator it = history.begin(); it != history.end(); ++it )
        {
            size_t hash = it->first;
            const time_list& times = it->second;
            os << hash;

            if ( times.empty() )
            {
                os << std::endl;
                continue;
            }

            os << " " << time_string( times[0] );

            for ( size_t i = 1; i < times.size(); ++i )
            {
                os << ", " << time_duration_string( times[i] - times[i - 1] );
            }

            os << std::endl;
        }

        return os;
    }


    std::string get_history_string( const history_type& history )
    {
        std::stringstream strm;
        output_history( strm, history );
        return strm.str();
    }


    std::ostream& output_time_list( std::ostream& os, const time_list& times )
    {
        if ( times.empty() )
        {
            return os;
        }

        os << " " << time_string( times[0] );

        for ( size_t i = 1; i < times.size(); ++i )
        {
            os << ", " << time_duration_string( times[i] - times[i - 1] );
        }

        return os;
    }


    std::string get_time_list_string( const time_list& times )
    {
        std::stringstream strm;
        output_time_list( strm, times );
        return strm.str();
    }


    std::string time_string( std::time_t t, const char* format )
    {
        std::tm* m = std::localtime( &t );
        char s[100] = { 0 };
        std::strftime( s, 100, format, m );
        return s;
    }


    bool is_today( std::time_t t )
    {
        static std::time_t today_t = std::time(0);
        static std::tm today_tm = *std::localtime( &today_t );
        std::tm* m = std::localtime( &t );
        return ( m->tm_mday == today_tm.tm_mday && m->tm_mon == today_tm.tm_mon && m->tm_year == today_tm.tm_year );
    }


    std::string time_duration_string( std::time_t t )
    {
        enum { minute = 60, hour = 60 * minute ,day = 24 * hour, month = 30 * day };

        std::stringstream strm;
        std::time_t mon = 0, d = 0, h = 0, min = 0;

        #define CALCULATE( x, u, n ) if ( u <= x  ) { n = x / u; x %= u; }
        CALCULATE( t, month, mon );
        CALCULATE( t, day, d );
        CALCULATE( t, hour, h );
        CALCULATE( t, minute, min );
        #undef CALCULATE

        #define WRAP_ZERO(x) (9 < x ? "" : "0") << x
        if ( mon || d ) { strm << WRAP_ZERO(mon) << "/" << WRAP_ZERO(d) << "-"; }
        strm << WRAP_ZERO(h) << ":" << WRAP_ZERO(min);
        #undef WRAP_ZERO

        return strm.str();
    }


    std::vector<std::string> extract_words( const std::string& s )
    {
        std::vector<std::string> words;
        static const boost::regex e( "(?x)\\{ ( [^{}]+ ) \\}" );
        boost::sregex_iterator it( s.begin(), s.end(), e );
        boost::sregex_iterator end;

        for ( ; it != end; ++it )
        {
            std::string w = boost::trim_copy( it->str(1) );

            if ( ! w.empty() )
            {
                words.push_back( w );
            }
        }

        return words;
    }


    void play_sound( const std::string& file  )
    {
        try
        {
            IGraphBuilder* graph = NULL;
            IMediaControl* control = NULL;
            IMediaEvent*   evnt = NULL;

            ::CoCreateInstance( CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void **)&graph );
            graph->QueryInterface( IID_IMediaControl, (void **)&control );
            graph->QueryInterface( IID_IMediaEvent, (void **)&evnt );

            HRESULT hr = graph->RenderFile( boost::locale::conv::utf_to_utf<wchar_t>(file).c_str(), NULL );

            if ( SUCCEEDED(hr) )
            {
                hr = control->Run();

                if ( SUCCEEDED(hr) )
                {
                    long code;
                    evnt->WaitForCompletion( INFINITE, &code );
                }
            }

            evnt->Release();
            control->Release();
            graph->Release();
        }
        catch ( ... )
        {
            LOG << "error " << file;
        }
    }


    //void play_sound_fucked( const std::string& file )
    //{
    //    char buffer[MAX_PATH] = { 0 };
    //    ::GetShortPathName( file.c_str(), buffer, MAX_PATH );
    //    std::string s = "play "+ std::string(buffer) + " wait";
    //    LOG_TRACE << "mciSendString " << s;

    //    MCIERROR code = ::mciSendString( s.c_str(), NULL, 0, NULL );

    //    if ( 0 != code )
    //    {
    //        LOG << "error: mciSendString file =" << file << ", error = " << code;
    //    }
    //}


    void play_sound_list( const std::vector<std::string>& files )
    {
        for ( size_t i = 0; i < files.size(); ++i )
        {
            play_sound( files[i] );
        }
    }


    void play_sound_list_thread( const std::vector<std::string>& files )
    {
        static QueueProcessor<> player( ( boost::function<void (const std::vector<std::string>&)>( &play_sound_list ) ) );
        player.queue_items( files );
    }


    void text_to_speech( const std::string& word )
    {
        std::vector<std::string> words;
        words.push_back( word );
        text_to_speech_list( words );
    }


    void text_to_speech_list( const std::vector<std::string>& words )
    {
        static ISpVoice* sp_voice = NULL;

        if ( NULL == sp_voice )
        {
            ::CoCreateInstance( CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void **)&sp_voice );
        }

        for ( size_t i = 0; i < words.size(); ++i )
        {
            // std::wstring ws = boost::locale::conv::utf_to_utf<wchar_t>( words[i], "" );
            std::wstring ws = boost::locale::conv::to_utf<wchar_t>( words[i], "GBK" );
            sp_voice->Speak( ws.c_str(), 0, NULL );
            LOG_TRACE << words[i];

            if ( i + 1 < words.size() )
            {
                boost::this_thread::sleep_for( boost::chrono::milliseconds(300) );
            }
        }
    }


    void text_to_speech_list_thread( const std::vector<std::string>& words )
    {
        static QueueProcessor<> speaker( ( boost::function<void (const std::vector<std::string>&)>( &text_to_speech_list ) ) );
        speaker.queue_items( words );
    }


    void play_or_tts( const std::pair<std::string, std::string>& word_path )
    {
        if ( ! word_path.second.empty() )
        {
            play_sound( word_path.second );
        }
        else
        {
            text_to_speech( word_path.first );
        }
    }


    void play_or_tts_list( const std::vector< std::pair<std::string, std::string> >& word_path_list )
    {
        for ( size_t i = 0; i < word_path_list.size(); ++i )
        {
            play_or_tts( word_path_list[i] );
        }
    }

    void play_or_tts_list_thread( const std::vector< std::pair<std::string, std::string> >& word_path_list )
    {
        static QueueProcessor< std::pair<std::string, std::string> > play_tts( ( boost::function<void (const std::vector< std::pair<std::string, std::string> >&)>( &play_or_tts_list ) ) );
        play_tts.queue_items( word_path_list );
    }


    RecordSound::RecordSound( const std::string& n )
        : m_file_name( n )
    {
        ::mciSendString( "set wave samplespersec 11025", "", 0, 0 );
        ::mciSendString( "set wave channels 2", "", 0, 0 );
        ::mciSendString( "close my_wav_sound", 0, 0, 0 );
        ::mciSendString( "open new type WAVEAudio alias my_wav_sound", 0, 0, 0 );
        ::mciSendString( "record my_wav_sound", 0, 0, 0 );
        LOG_DEBUG << "recording bein" << m_file_name;
    }


    RecordSound::~RecordSound()
    {
        std::string s = "save my_wav_sound " + m_file_name;
        ::mciSendString( "stop my_wav_sound", 0, 0, 0 );
        ::mciSendString( s.c_str(), 0, 0, 0 );
        ::mciSendString( "close my_wav_sound", 0, 0, 0 );
        LOG_DEBUG << "recording end" << m_file_name;
    }


    size_t random_number( size_t lo, size_t hi )
    {
        static boost::random::mt19937 gen( static_cast<boost::uint32_t>( std::time(0) ) );
        static boost::random::uniform_int_distribution<> dist;
        size_t x = dist( gen );

        if ( lo != std::numeric_limits <size_t> ::min() || hi != std::numeric_limits <size_t> ::max() )
        {
            while ( x < lo || hi < x )
            {
                x %= hi;
                x += lo;
            }
        }

        LOG_TRACE << x << "(low = " << lo << ", high = " << hi << ")";
        return x;
    }

}
