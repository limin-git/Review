#pragma once


class SingleLineReview
{
public:

    SingleLineReview( const std::string& file_name );

public:

    void review();

public:

    bool reload_history();
    bool check_last_write_time();
    bool reload_strings();
    void synchronize_history();
    std::vector< std::pair<std::string, size_t> > collect_strings_to_review();
    void display_string_to_review( const std::vector< std::pair<std::string, size_t> >& strings, size_t index );
    void wait_for_input();
    void save_review_time( const std::pair<std::string, size_t>& s );
    void save_review_to_history();

public:

    std::string m_file_name;
    std::string m_review_name;
    std::string m_history_name;
    std::ofstream m_review_strm;
    std::time_t m_last_modified_time;
    std::vector<size_t> m_review_span;
    boost::log::sources::logger m_log;
    boost::hash<std::string> m_string_hash;
    std::map< size_t, std::vector<std::time_t> > m_history;
    std::vector< std::pair<std::string, size_t> > m_strings;
};
