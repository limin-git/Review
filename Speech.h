#pragma once


class Speech
{
public:

    Speech();
    void play( const std::vector<std::string>& words );
    std::vector<std::string> get_files( const std::string& word );
    std::vector<std::string> get_files( const std::vector<std::string>& words, std::vector<std::string>& speak_words );
    void update_option( const boost::program_options::variables_map& vm ); // ProgramOptions slot

public:

    boost::mutex m_mutex;
    bool m_no_duplicate;
    bool m_no_text_to_speech;
    size_t m_text_to_speech_repeat;
    std::vector<boost::filesystem::path> m_paths;
    std::vector<std::string> m_speech_path_option;
};
