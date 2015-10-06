#pragma once


class SingleLineReview
{
public:

    SingleLineReview( const std::string& file_name, const boost::program_options::variables_map& vm = boost::program_options::variables_map() );

public:

    void review();

public:

    bool reload_history();
    bool check_last_write_time();
    bool reload_strings();
    void synchronize_history();
    void collect_strings_to_review();
    void collect_strings_to_review_thread();
    void display_reviewing_string( const std::vector< std::pair<std::string, size_t> >& strings, size_t index );
    void wait_for_input( const std::string& message = "" );
    void save_review_time( const std::pair<std::string, size_t>& s );
    void save_review_to_history();
    void on_review_begin();
    void on_review_end();

public:

    boost::mutex m_mutex;
    std::string m_file_name;
    std::string m_review_name;
    std::string m_history_name;
    std::ofstream m_review_strm;
    volatile bool m_is_reviewing;
    std::time_t m_last_modified_time;
    std::vector<size_t> m_review_span;
    boost::log::sources::logger m_log;
    boost::hash<std::string> m_string_hash;
    boost::program_options::variables_map m_variable_map;
    std::map< size_t, std::vector<std::time_t> > m_history;
    std::vector< std::pair<std::string, size_t> > m_strings;
    std::vector< std::pair<std::string, size_t> > m_reviewing_strings;
};
