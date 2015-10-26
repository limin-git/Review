#pragma once


class Speech
{
public:

    Speech();
    void play( const std::string& word );
    void play( const std::vector<std::string>& words );
    std::vector<std::string> get_files( const std::string& word );
    std::vector<std::string> get_files( const std::vector<std::string>& words );
    void update_option( const boost::program_options::variables_map& vm );

public:

    boost::mutex m_mutex;
    bool m_no_duplicate;
    std::vector<boost::filesystem::path> m_paths;
    std::vector<std::string> m_speech_path_option;
};
