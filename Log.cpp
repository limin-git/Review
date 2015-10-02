#include "stdafx.h"
#include "Log.h"


boost::program_options::options_description Log::get_description()
{
    boost::program_options::options_description desc( "Log Options" );
    desc.add_options()
        ( "log-ini", boost::program_options::value<std::string>(), "log configuration file" )
        ( "log-file-name", boost::program_options::value<std::string>(), "log file name." )
        ( "log-rotation-size", boost::program_options::value<size_t>(), "log rotation size." )
        ;

    return desc;
}


void Log::initialize( const boost::program_options::variables_map& vm )
{
    using namespace boost::log;

    std::string log_ini;
    std::string log_file_name;
    size_t log_rotation_size = 20 * 1024 * 1024;

    if ( vm.count( "log-ini" ) )
    {
        log_ini = vm["log-ini"].as<std::string>();
    }

    if ( vm.count( "log-file-name" ) )
    {
        log_file_name = vm["log-file-name"].as<std::string>();
    }

    if ( vm.count( "log-rotation-size" ) )
    {
        log_rotation_size = vm["log-rotation-size"].as<size_t>();
    }

    try
    {
        if ( false == log_ini.empty() )
        {
            std::ifstream is( log_ini.c_str() );
            boost::log::init_from_stream( is );
        }
        else
        {
            // add sinks: console sink, file sink
            boost::shared_ptr< sinks::synchronous_sink<sinks::text_ostream_backend> > console_sink = add_console_log();

            // set formatter
            formatter frmttr = expressions::stream
                << "\t"
                << expressions::format_date_time<boost::posix_time::ptime>( "TimeStamp", "%Y/%m/%d %H:%M:%S " )
                << expressions::if_( expressions::has_attr<int>( "Severity" ) )
                   [
                       expressions::stream << "[" << expressions::attr<int>( "Severity" ) << "] "
                   ]
                << expressions::if_( expressions::has_attr<std::string>( "Process" ) )
                   [
                       expressions::stream << "[" << expressions::attr<std::string>( "Process" ) << "] "
                   ]
                //<< "[" << expressions::attr<process_id>("ProcessID") << "] "
                //<< "[" << expressions::attr<thread_id>( "ThreadID" ) << "] "
                << expressions::format_named_scope( "Scope", keywords::format = "%c" )
                << expressions::if_( expressions::has_attr<std::string>( "Channel" ) )
                   [
                       expressions::stream << "[" << expressions::attr<std::string>( "Channel" ) << "] "
                   ]
                << expressions::if_( expressions::has_attr<std::string>( "Tag" ) )
                   [
                       expressions::stream << "[" << expressions::attr<std::string>( "Tag" ) << "] "
                   ]
                << expressions::if_( expressions::has_attr<boost::posix_time::time_duration>( "Duration" ) )
                   [
                       expressions::stream << "[" << expressions::attr<boost::posix_time::time_duration>( "Duration" ) << "] "
                   ]
                << " "
                << expressions::message;

            console_sink->set_formatter( frmttr );

            if ( false == log_file_name.empty() )
            {
                boost::shared_ptr< sinks::synchronous_sink<sinks::text_file_backend> > file_sink =
                    boost::log::add_file_log
                    (
                        keywords::file_name = log_file_name.c_str(),
                        keywords::rotation_size = log_rotation_size,
                        keywords::auto_flush = true
                    );

                file_sink->set_formatter( frmttr );
            }
        }

        add_common_attributes();
        //core::get()->add_global_attribute( "Scope", attributes::named_scope() );
        //core::get()->add_global_attribute( "Process", attributes::current_process_name() );
    }
    catch ( std::exception& e )
    {
        std::cout << __FUNCDNAME__ << " - " << e.what() << std::endl;
    }
}
