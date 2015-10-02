#include "stdafx.h"
#include "SingleLineReview.h"
#include "Log.h"


int _tmain(int argc, _TCHAR* argv[])
{
    boost::program_options::variables_map vm;
    boost::program_options::options_description desc( "Options" );
    desc.add_options()
        ( "help,?", "produce help message" )
        ( "file-name,F", boost::program_options::value<std::string>(),  "the file to be reviewed" )
        ;

    desc.add( Log::get_description() );
    boost::program_options::store( boost::program_options::command_line_parser(argc, argv).options(desc).allow_unregistered().run(), vm );
    boost::program_options::notify(vm);

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

    try
    {
        SingleLineReview( file_name ).review();
    }
    catch ( ... )
    {
        std::cout << "caught exception, exit." << std::endl;
    }

	return 0;
}
