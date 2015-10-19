#pragma once
#include "ReviewString.h"
class Loader;
class History;


class ReviewManager
{
public:

    enum EReviewDirection{ forward, backward };

public:

    ReviewManager( const boost::program_options::variables_map& vm );
    void review();

public:

    ReviewString get_next();
    ReviewString get_previous();
    std::string wait_for_input( const std::string& message = "" );
    void set_title();
    void update();
    void update_thread();

public:

    std::ostream& output_hash_list( std::ostream& os, const std::list<size_t>& l );
    std::string get_hash_list_string( const std::list<size_t>& l );

public:

    boost::mutex m_mutex;
    std::string m_file_name;
    std::string m_history_name;
    Loader* m_loader;
    History* m_history;
    std::set<size_t> m_all;
    std::set<size_t> m_reviewing_set;
    std::list<size_t> m_reviewing_list;
    EReviewDirection m_review_mode;
    size_t m_backward_index;
    std::vector<size_t> m_review_history;
    boost::timer::nanosecond_type m_minimal_review_time;
    size_t m_auto_update_interval;
    boost::program_options::variables_map m_variables_map;
};
