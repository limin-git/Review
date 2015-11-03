#include "stdafx.h"
#include "ConsoleCommand.h"
#include "OptionString.h"
#include "OptionUpdateHelper.h"
#include "ProgramOptions.h"


ConsoleCommand::ConsoleCommand()
{
    ProgramOptions::connect_to_signal( boost::bind( &ConsoleCommand::update_option, this, _1 ) );
}


void ConsoleCommand::update_option( const boost::program_options::variables_map& vm )
{
    static OptionUpdateHelper option_helper;

    if ( option_helper.update_one_option< std::vector<std::string> >( system_command_option, vm ) )
    {
        m_commands = option_helper.get_value< std::vector<std::string> >( system_command_option );

        for ( size_t i = 0; i < m_commands.size(); ++i )
        {
            std::system( m_commands[i].c_str() );
            LOG_DEBUG << m_commands[i];
        }
    }
}
