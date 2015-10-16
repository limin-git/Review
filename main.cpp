#include "stdafx.h"
#include "Log.h"
#include "ReviewManager.h"


int _tmain(int argc, _TCHAR* argv[])
{
    system( "COLOR F0" );

    boost::program_options::variables_map vm;
    boost::program_options::options_description desc( "Options", 100 );
    desc.add_options()
        ( "help,?", "produce help message" )
        ( "file-name,F", boost::program_options::value<std::string>(),  "the file to be reviewed" )
        ( "history-file-name,H", boost::program_options::value<std::string>(),  "history" )
        ( "review-file-name,R", boost::program_options::value<std::string>(),  ".review, history cache" )
        ( "config-file,C", boost::program_options::value<std::string>()->default_value( "review.cfg" ),  "config file" )
        ( "display-mode,M", boost::program_options::value<std::string>()->default_value( "all" ),  "(NOT_IMPLEMENT_YET) all / question-only / answer-only" )
        ( "review-time-span-list,T", boost::program_options::value< std::vector<std::string> >()->multitoken(),  "review time span list" )
        ( "minimal-review-time", boost::program_options::value<size_t>()->default_value( 500 ),  "in miniseconds" )
        ( "max-cache-size", boost::program_options::value<size_t>()->default_value( 100 ),  "normally write to .review, write to .history every max-cache-size times" )
        ( "auto-update-interval,A", boost::program_options::value<size_t>()->default_value( 60 ),  "in seconds" )
        ;

    desc.add( Log::get_description() );
    store( boost::program_options::command_line_parser(argc, argv).options(desc).allow_unregistered().run(), vm );
    notify( vm );

    if ( vm.count( "config-file" ) )
    {
        std::string config_file = vm["config-file"].as<std::string>();

        if ( boost::filesystem::exists( config_file ) )
        {
            store( boost::program_options::parse_config_file<char>( config_file.c_str(), desc, true ), vm );
            notify( vm );
        }
    }

    if ( vm.count( "help" ) )
    {
        std::cout << desc << std::endl;
        return 0;
    }

    Log::initialize( vm );
    boost::log::sources::logger log;

    if ( ! vm.count( "file-name" ) )
    {
        BOOST_LOG(log) << "the file-name must be set";
        std::cout << desc << std::endl;
        return 0;
    }

    std::string file_name = vm["file-name"].as<std::string>();
    system( ("TITLE " + file_name).c_str() );

    if ( ::CreateMutex( NULL, FALSE, file_name.c_str() ) == NULL || GetLastError() == ERROR_ALREADY_EXISTS )
    {
        std::cout << "another instance is running." << std::endl;
        return 0;
    }

    try
    {
        ReviewManager rm( vm );
        rm.review();
    }
    catch ( ... )
    {
        std::cout << "caught exception, exit." << std::endl;
    }

	return 0;
}
