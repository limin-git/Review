#pragma once
class History;
class Loader;
class Speech;


class ReviewString
{
public:

    ReviewString( size_t hash = 0, Loader* loader = NULL, History* history = NULL, Speech* play = NULL, bool answer_first = false );
    std::string review();
    void play_speech();
    const std::string& get_string();
    size_t get_hash() { return m_hash; }

public:

    size_t m_hash;
    Loader* m_loader;
    History* m_history;
    Speech* m_speech;
    bool m_answer_first;
};
