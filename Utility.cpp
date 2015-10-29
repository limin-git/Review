#include "stdafx.h"
#include "Utility.h"
#include "Log.h"


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


    std::string time_duration_string( std::time_t t )
    {
        enum { minute = 60, hour = 60 * minute ,day = 24 * hour, month = 30 * day };

        std::stringstream strm;
        std::time_t mon = 0, d = 0, h = 0, min = 0;

        #define CALCULATE( n, u, x ) if ( u <= x  ) { n = x / u; x %= u; }
        CALCULATE( mon, month, t );
        CALCULATE( d, day, t );
        CALCULATE( h, hour, t );
        CALCULATE( min, minute, t );
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


    void play_sound( const std::string& file )
    {
        char buffer[MAX_PATH] = { 0 };
        GetShortPathName( file.c_str(), buffer, MAX_PATH );
        std::string s = "play "+ std::string(buffer) + " wait";
        LOG_TRACE << "mciSendString " << s;

        MCIERROR code = ::mciSendString( s.c_str(), NULL, 0, NULL );

        if ( 0 != code )
        {
            LOG << "error: mciSendString file =" << file << ", error = " << code;
        }
    }


    void play_sounds( const std::vector<std::string>& files )
    {
        for ( size_t i = 0; i < files.size(); ++i )
        {
            play_sound( files[i] );
        }
    }


    void play_sound_thread( const std::string& file )
    {
        char buffer[MAX_PATH] = { 0 };
        GetShortPathName( file.c_str(), buffer, MAX_PATH );
        std::string s = "play "+ std::string(buffer) + " wait";
        LOG_TRACE << "mciSendString " << s;

        MCIERROR code = ::mciSendString( s.c_str(), NULL, 0, NULL );

        if ( 0 != code )
        {
            LOG << "error: mciSendString file =" << file << ", error = " << code;
        }
    }


    void play_sound_thread( const std::vector<std::string>& files )
    {
        struct Player
        {
            void add_files( const std::vector<std::string>& files )
            {
                boost::unique_lock<boost::mutex> lock( m_mutex );
                m_files.insert( m_files.end(), files.begin(), files.end() );
                m_condition.notify_one();
            }

            void operator()()
            {
                while ( true )
                {
                    {
                        boost::unique_lock<boost::mutex> lock( m_mutex );

                        while ( m_files.empty() )
                        {
                            m_condition.wait( lock );
                        }

                        m_playing.swap( m_files );
                    }

                    play_sounds( m_playing );
                    m_playing.clear();
                }
            }

            boost::mutex m_mutex;
            boost::condition_variable m_condition;
            std::vector<std::string> m_files;
            std::vector<std::string> m_playing;
        };

        static Player player;
        static boost::thread t( boost::ref( player ) );
        player.add_files( files );
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

        if ( lo == std::numeric_limits <size_t> ::min() && hi == std::numeric_limits <size_t> ::max() )
        {
            return x;
        }

        while ( x < lo || hi < x )
        {
            x %= hi;
            x += lo;
        }

        return x;
    }

}
