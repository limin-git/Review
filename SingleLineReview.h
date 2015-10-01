#pragma once


class SingleLineReview
{
public:

    SingleLineReview( const std::string& file_name );
    bool initialize();
    void review();
    void save_review_to_history();
    std::vector< std::pair<std::string, size_t> > collect_strings_to_review();

public:

    void save_review_time( const std::pair<std::string, size_t>& s );
    bool load_strings();
    bool load_history();

public:

    std::string m_file_name;
    std::string m_review_name;
    std::string m_history_name;
    std::string m_ignore_name;
    std::ofstream m_review_strm;
    boost::log::sources::logger m_log;
    std::vector< std::pair<std::string, size_t> > m_strings;
    std::map< size_t, std::vector<std::time_t> > m_history;
};
