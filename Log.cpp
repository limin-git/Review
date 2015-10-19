#include "stdafx.h"
#include "Log.h"
BOOST_LOG_ATTRIBUTE_KEYWORD(debug_level_attribute, "Level", std::string)


boost::log::sources::logger m_log;
boost::log::sources::logger m_log_error;
boost::log::sources::logger m_log_info;
boost::log::sources::logger m_log_debug;
boost::log::sources::logger m_log_trace;
boost::log::sources::logger m_log_test;


boost::program_options::options_description Log::get_description()
{
    boost::program_options::options_description desc( "Log Options" );
    desc.add_options()
        ( "log-ini", boost::program_options::value<std::string>(), "log configuration file" )
        ( "log-file", boost::program_options::value<std::string>(), "log file name." )
        ( "log-rotation-size", boost::program_options::value<size_t>(), "log rotation size." )
        ( "log-levels", boost::program_options::value< std::vector<std::string> >()->multitoken(), "error, info, debug, trace, *" )
        ( "log-no-console", "no console output" )
        ;

    return desc;
}


void Log::initialize( const boost::program_options::variables_map& vm )
{
    using namespace boost::log;

    m_log_error.add_attribute( "Level", boost::log::attributes::constant<std::string>( "ERROR" ) );
    m_log_info.add_attribute( "Level", boost::log::attributes::constant<std::string>( "INFO" ) );
    m_log_debug.add_attribute( "Level", boost::log::attributes::constant<std::string>( "DEBUG" ) );
    m_log_trace.add_attribute( "Level", boost::log::attributes::constant<std::string>( "TRACE" ) );
    m_log_test.add_attribute( "Level", boost::log::attributes::constant<std::string>( "TEST" ) );

    std::string log_ini;
    std::string log_file_name;
    size_t log_rotation_size = 20 * 1024 * 1024;

    if ( vm.count( "log-ini" ) )
    {
        log_ini = vm["log-ini"].as<std::string>();
    }

    if ( vm.count( "log-file" ) )
    {
        log_file_name = vm["log-file"].as<std::string>();
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
                << expressions::if_( expressions::has_attr<std::string>( "Level" ) )
                    [
                        expressions::stream << "[" << expressions::attr<std::string>( "Level" ) << "] "
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

            std::vector<std::string> debug_levels;

            if ( vm.count( "log-levels" ) )
            {
                debug_levels = vm["log-levels"].as< std::vector<std::string> >();
            }

            bool with_error = std::find( debug_levels.begin(), debug_levels.end(), "error" )    != debug_levels.end();
            bool with_info  = std::find( debug_levels.begin(), debug_levels.end(), "info" )     != debug_levels.end();
            bool with_debug = std::find( debug_levels.begin(), debug_levels.end(), "debug" )    != debug_levels.end();
            bool with_trace = std::find( debug_levels.begin(), debug_levels.end(), "trace" )    != debug_levels.end();
            bool with_test  = std::find( debug_levels.begin(), debug_levels.end(), "test" )     != debug_levels.end();
            bool with_all   = std::find( debug_levels.begin(), debug_levels.end(), "*" )        != debug_levels.end();

            filter fltr = 
                ( ! expressions::has_attr(debug_level_attribute) ) || 
                ( expressions::has_attr(debug_level_attribute) && debug_level_attribute == "ERROR" && ( with_error || with_all ) ) ||
                ( expressions::has_attr(debug_level_attribute) && debug_level_attribute == "INFO"  && ( with_info  || with_all ) ) ||
                ( expressions::has_attr(debug_level_attribute) && debug_level_attribute == "DEBUG" && ( with_debug || with_all ) ) ||
                ( expressions::has_attr(debug_level_attribute) && debug_level_attribute == "TRACE" && ( with_trace || with_all ) ) ||
                ( expressions::has_attr(debug_level_attribute) && debug_level_attribute == "TEST" &&  ( with_test  || with_all ) )
                ;

            if ( ! vm.count( "log-no-console" ) )
            {
                // add sinks: console sink, file sink
                boost::shared_ptr< sinks::synchronous_sink<sinks::text_ostream_backend> > console_sink = add_console_log();
                console_sink->set_formatter( frmttr );
                console_sink->set_filter( fltr );
            }

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
                file_sink->set_filter( fltr  );
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
