#include "stdafx.h"
#include "Log.h"
#include "ReviewManager.h"
#include "OptionString.h"
#include "ConfigFileMonitor.h"
#include <objbase.h>


int main(int argc, char* argv[])
{
    system( "COLOR F0" );

    boost::program_options::variables_map vm;
    boost::program_options::options_description desc( "Options", 100 );
    desc.add_options()
        ( "help,?", "produce help message" )
        ( file_name_option, boost::program_options::value<std::string>(),  "the file to be reviewed" )
        ( file_history_option, boost::program_options::value<std::string>(),  "history" )
        ( file_review_option, boost::program_options::value<std::string>(),  ".review, history cache" )
        ( config_file_option, boost::program_options::value<std::string>()->default_value( "review.cfg" ),  "config file" )
        ( review_time_span_list_option, boost::program_options::value< std::vector<std::string> >()->multitoken(),  "review time span list" )
        ( review_minimal_time_option, boost::program_options::value<boost::timer::nanosecond_type>()->default_value( 500 ),  "in miniseconds" )
        ( review_max_cache_size_option, boost::program_options::value<size_t>()->default_value( 100 ),  "normally write to .review, write to .history every max-cache-size times" )
        ( review_auto_update_interval_option, boost::program_options::value<size_t>()->default_value( 60 ),  "in seconds" )
        ( speech_play_back, boost::program_options::value<size_t>()->default_value( 0 ),  "listen back" )
        ( speech_disabled_option, boost::program_options::value<std::string>()->default_value("true"), "true or false" )
        ( speech_path_option, boost::program_options::value< std::vector<std::string> >()->multitoken(), "speech path" )
        ;

    desc.add( Log::get_description() );
    store( boost::program_options::command_line_parser(argc, argv).options(desc).allow_unregistered().run(), vm );
    notify( vm );

    if ( vm.count( config_file_option ) )
    {
        std::string config = vm[config_file_option].as<std::string>();

        if ( boost::filesystem::exists( config ) )
        {
            store( boost::program_options::parse_config_file<char>( config.c_str(), desc, true ), vm );
            notify( vm );
        }

        ConfigFileMonitor::initialize( config, desc );
    }

    if ( vm.count( "help" ) )
    {
        std::cout << desc << std::endl;
        return 0;
    }

    Log::initialize( vm );
    boost::log::sources::logger log;

    if ( ! vm.count( file_name_option ) )
    {
        LOG << "the file-name must be set";
        std::cout << desc << std::endl;
        return 0;
    }

    std::string file = vm[file_name_option].as<std::string>();
    system( ("TITLE " + file).c_str() );

    if ( ::CreateMutex( NULL, FALSE, file.c_str() ) == NULL || GetLastError() == ERROR_ALREADY_EXISTS )
    {
        std::cout << "another instance is running." << std::endl;
        return 0;
    }

    try
    {
        ::CoInitializeEx( NULL, COINIT_MULTITHREADED );
        ReviewManager rm( vm );
        rm.review();
    }
    catch ( boost::filesystem::filesystem_error& e )
    {
        std::cout << e.what() << std::endl;
    }
    catch ( std::exception& e )
    {
        std::cout << e.what() << std::endl;
    }
    catch ( ... )
    {
        std::cout << "caught exception, exit." << std::endl;
    }

	return 0;
}
