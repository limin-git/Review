#pragma once
class History;
class Loader;
class Speech;


class ReviewString
{
public:

    ReviewString( size_t hash = 0, Loader* loader = NULL, History* history = NULL, Speech* play = NULL );
    std::string review();
    void play_speech();

public:

    size_t m_hash;
    Loader* m_loader;
    History* m_history;
    Speech* m_speech;
};
