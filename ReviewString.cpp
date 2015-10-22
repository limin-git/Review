#include "stdafx.h"
#include "ReviewString.h"
#include "Loader.h"
#include "History.h"
#include "Speech.h"
#include "Utility.h"
#include "Log.h"


ReviewString::ReviewString( size_t hash, Loader* loader, History* history, Speech* play )
    : m_hash( hash ),
      m_loader( loader ),
      m_history( history ),
      m_speech( play )
{
}


std::string ReviewString::review()
{
    if ( 0 == m_hash )
    {
        std::cout << "empty." << std::flush;
        return "";
    }

    std::string s = m_loader->get_string( m_hash );
    boost::erase_all( s, "{" );
    boost::erase_all( s, "}" );

    if ( boost::starts_with( s, "[Q]" ) || boost::starts_with( s, "[q]" ) )
    {
        size_t pos = s.find( "[A]" );

        if ( pos == std::string::npos )
        {
            pos = s.find( "[a]" );

            if ( pos == std::string::npos )
            {
                std::cout << __FUNCTION__ << " - bad format: " << s;
                return "";
            }
        }

        std::string q = s.substr( 4, pos - 4 );
        std::string a = s.substr( pos + 4 );
        boost::trim(q);
        boost::trim(a);

        if ( !q.empty() && ! a.empty() )
        {
            std::cout << "\t" << q << std::flush;

            std::string command;
            std::getline( std::cin, command );
            if ( ! command.empty() )
            {
                return command;
            }

            std::cout << "\t" << a << std::flush;
        }
        else
        {
            std::cout << "bad format: question " << ( q.empty() ? "is" : "is not") << " empty, answer " << (a.empty() ? "is." : "is not") << " empty. " << s << std::endl; 
        }
    }
    else
    {
        std::cout << "\t" << s << std::flush;
    }

    if ( m_speech )
    {
        play_speech();
    }

    LOG_DEBUG
        << s << std::endl
        << "(round: " << m_history->get_review_round( m_hash ) << ") "
        << Utility::get_time_list_string( m_history->get_times( m_hash ) );
    return "";
}


void ReviewString::play_speech()
{
    const std::string& s = m_loader->get_string( m_hash );
    static const boost::regex e( "(?x)\\{ ( [^{}]+ ) \\}" );
    boost::sregex_iterator it( s.begin(), s.end(), e );
    boost::sregex_iterator end;
    std::vector<std::string> words;

    for ( ; it != end; ++it )
    {
        words.push_back( boost::trim_copy(it->str(1)) );
    }

    if ( ! words.empty() )
    {
        m_speech->play( words );
    }
}
