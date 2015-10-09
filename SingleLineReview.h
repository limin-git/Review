#pragma once
typedef std::vector<std::time_t> time_list;
typedef std::map<size_t, time_list> history_type;


class SingleLineReview
{
    enum { minute = 60, hour = 60 * minute ,day = 24 * hour, month = 30 * day };

public:

    SingleLineReview( const std::string& file_name, const boost::program_options::variables_map& vm = boost::program_options::variables_map() );

public:

    void review();

public:

    void initialize_history();
    bool reload_strings();
    void synchronize_history();
    void collect_strings_to_review();
    void collect_strings_to_review_thread();
    void display_reviewing_string( const std::vector< std::pair<std::string, size_t> >& strings, size_t index );
    void wait_for_input( const std::string& message = "" );
    void write_review_time( const std::pair<std::string, size_t>& s );
    void write_review_to_history();
    void on_review_begin();
    void on_review_end();
    bool write_history();
    void merge_history( const history_type& review_history );
    std::ostream& output_history( std::ostream& os, const history_type& history );
    std::string get_history_string( const history_type& history );

public:

    history_type load_history_from_file( const std::string& file_name );

public:

    std::string time_string( std::time_t t, const char* format = "%Y/%m/%d %H:%M:%S" );
    std::string time_duration_string( std::time_t t );

public:

    boost::mutex m_mutex;
    history_type m_history;
    std::string m_file_name;
    std::string m_review_name;
    std::string m_history_name;
    std::ofstream m_review_strm;
    volatile bool m_is_reviewing;
    std::vector<std::time_t> m_review_span;
    boost::log::sources::logger m_log;
    boost::log::sources::logger m_log_debug;
    boost::log::sources::logger m_log_trace;
    boost::hash<std::string> m_string_hash;
    boost::program_options::variables_map m_variable_map;
    std::vector< std::pair<std::string, size_t> > m_strings;
    std::vector< std::pair<std::string, size_t> > m_reviewing_strings;
};
