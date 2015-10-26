#include "stdafx.h"
#include "Utility.h"
#include "Log.h"
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


    void play_sounds( const std::vector<std::string>& files )
    {
        for ( size_t i = 0; i < files.size(); ++i )
        {
            play_sound( files[i] );
        }
    }


    void play_sound_thread( const std::string& file )
    {
        boost::thread( boost::bind( &play_sound, file ) );
    }


    void play_sound_thread( const std::vector<std::string>& files )
    {
        boost::thread( boost::bind( &play_sounds, files ) );
    }

}
