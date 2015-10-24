#pragma once
typedef boost::signals2::signal<void (const boost::program_options::variables_map&)> signal_type;
typedef signal_type::slot_type slot_type;


class ConfigFileMonitor
{
public:

    static void initialize( const std::string& file, const boost::program_options::options_description& desc );
    static void scan_file();
    static void scan_file_thread();
    static boost::signals2::connection  connect_to_signal( slot_type slot );

public:

    static std::string m_file;
    static std::time_t m_last_write_time;
    static signal_type m_signal;
    static size_t m_interval;
    static boost::program_options::variables_map m_vm;
    static const boost::program_options::options_description* m_desc;
};
