#include "stdafx.h"
#include "ConfigFileMonitor.h"
#include "Log.h"
#include "Utility.h"


std::string ConfigFileMonitor::m_file;
std::time_t ConfigFileMonitor::m_last_write_time;
signal_type ConfigFileMonitor::m_signal;
size_t ConfigFileMonitor::m_interval;
boost::program_options::variables_map ConfigFileMonitor::m_vm;
const boost::program_options::options_description* ConfigFileMonitor::m_desc;


void ConfigFileMonitor::initialize( const std::string& file, const boost::program_options::options_description& desc )
{
    m_file = file;
    m_desc = &desc;
    m_last_write_time = 0;
    m_interval = 60;
    // new boost::thread( &ConfigFileMonitor::scan_file_thread );
}


void ConfigFileMonitor::scan_file()
{
    if ( ! boost::filesystem::exists( m_file ) )
    {
        return;
    }

    std::time_t t = boost::filesystem::last_write_time( m_file );

    if ( t == m_last_write_time )
    {
        return;
    }

    LOG_DEBUG << m_file;

    m_vm.clear();
    store( boost::program_options::parse_config_file<char>( m_file.c_str(), *m_desc, true ), m_vm );
    notify( m_vm );
    m_signal( m_vm );
    m_last_write_time = t;
}


boost::signals2::connection  ConfigFileMonitor::connect_to_signal( slot_type slot )
{
    return m_signal.connect( slot );
}


void ConfigFileMonitor::scan_file_thread()
{
    while ( true )
    {
        boost::this_thread::sleep_for( boost::chrono::seconds( m_interval ) );
        scan_file();
        LOG_DEBUG << m_file;
    }
}
