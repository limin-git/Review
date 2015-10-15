#include "stdafx.h"
#include "ReviewString.h"
#include "Loader.h"
#include "History.h"


ReviewString::ReviewString( size_t hash, Loader* loader, History* history )
    : m_hash( hash ),
      m_loader( loader ),
      m_history( history )
{
}


ReviewString::~ReviewString()
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
    }
    else
    {
        std::cout << "\t" << s << std::flush;
    }

    return "";
}
