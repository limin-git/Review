#include "stdafx.h"
#include "ReviewString.h"
#include "Loader.h"
#include "History.h"
#include "Utility.h"


extern boost::log::sources::logger m_log_debug;


ReviewString::ReviewString( size_t hash, Loader* loader, History* history )
    : m_hash( hash ),
      m_loader( loader ),
      m_history( history )
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

    BOOST_LOG(m_log_debug) << __FUNCTION__ << " - "
        << s << std::endl
        << "(round: " << m_history->get_review_round( m_hash ) << ") "
        << Utility::get_time_list_string( m_history->get_times( m_hash ) );
    return "";
}
