#pragma once


class Loader
{
public:

    Loader();
    const std::set<size_t>& get_string_hash_set();
    const std::string& get_string( size_t hash );

public:

    void reload();
    void process_file_change();

public:

    static size_t string_hash( const std::string& str );

public:

    static std::string get_difference( const std::set<size_t>& os, const std::map<size_t, std::string>& om, const std::set<size_t>& ns, const std::map<size_t, std::string>& nm  );

public:

    boost::mutex m_mutex;
    std::string m_file_name;
    std::time_t m_last_write_time;
    std::set<size_t> m_string_hash_set;
    std::map<size_t, std::string> m_hash_2_string_map;
};
