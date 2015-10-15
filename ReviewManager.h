#pragma once
#include "ReviewString.h"
class Loader;
class History;


class ReviewManager
{
public:

    ReviewManager( const boost::program_options::variables_map& vm );
    void review();

public:

    ReviewString get_next();
    std::string wait_for_input( const std::string& message = "" );
    void set_title();
    void update();
    void update_thread();

public:

    boost::mutex m_mutex;
    size_t m_minimal_review_time;
    size_t m_auto_update_interval;
    std::set<size_t> m_reviewing_set;
    std::set<size_t> m_all;
    std::list<size_t> m_reviewing_list;
    std::vector<size_t> m_review_history;
    Loader* m_loader;
    History* m_history;
    std::string m_file_name;
    std::string m_history_name;
    boost::program_options::variables_map m_variables_map;
};
