#pragma once
#include "ReviewString.h"
class Loader;
class History;
class Speech;


class ReviewManager
{
public:

    enum EReviewDirection{ Forward, Backward };

public:

    ReviewManager();
    void review();
    void listen_thread();

public:

    ReviewString get_next();
    ReviewString get_previous();
    std::string wait_for_input( const std::string& message = "" );
    void set_title();
    void update();
    void update_thread();
    void update_option( const boost::program_options::variables_map& vm ); // ProgramOptions slot

public:

    std::ostream& output_hash_list( std::ostream& os, const std::list<size_t>& l );
    std::string get_hash_list_string( const std::list<size_t>& l );

public:

    std::string get_new_expired_string( const std::set<size_t>& os,  const std::set<size_t>& ns );

public:

    size_t m_play_back;
    volatile bool m_is_listening;
    boost::condition_variable m_condition;
    boost::mutex m_mutex;
    std::string m_file_name;
    std::string m_history_name;
    Loader* m_loader;
    History* m_history;
    Speech* m_speech;
    Speech* m_speech_impl;
    std::set<size_t> m_all;
    std::set<size_t> m_reviewing_set;
    std::list<size_t> m_reviewing_list;
    EReviewDirection m_review_mode;
    size_t m_backward_index;
    std::vector<size_t> m_review_history;
    boost::timer::nanosecond_type m_minimal_review_time;
    size_t m_auto_update_interval;
    volatile ReviewString* m_current_reviewing;
};
