#pragma once
class History;
class Loader;


class ReviewString
{
public:

    ReviewString( size_t hash = 0, Loader* loader = NULL, History* history = NULL );
    std::string review();

public:

    size_t m_hash;
    Loader* m_loader;
    History* m_history;
    boost::log::sources::logger m_log_test;
};
