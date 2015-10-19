#pragma once
typedef std::vector<std::time_t> time_list;
typedef std::map<size_t, time_list> history_type;


class History
{
public:

    History( const boost::program_options::variables_map& vm );
    void initialize();
    void save_history( size_t hash );
    void synchronize_history( const std::set<size_t>& hashes );
    std::set<size_t> get_expired();
    size_t get_review_round( size_t hash ) { return m_history[hash].size(); }
    time_list& get_times( size_t hash ) { return m_history[hash]; }
    std::time_t get_last_review_time( size_t hash ) { time_list& t = m_history[hash]; return t.empty() ? 0 : t.back(); }

public:

    void write_history();
    void merge_history( const history_type& history );
    bool is_expired( size_t hash );
    void clean_review_cache();

public:

    history_type load_history_from_file( const std::string& file_name );

public:

    std::string m_file_name;
    std::string m_review_name;
    history_type m_history;
    std::ofstream m_review_stream;
    size_t m_max_cache_size;
    size_t m_cache_size;
    time_list m_review_spans;
    boost::program_options::variables_map m_variables_map;
};
