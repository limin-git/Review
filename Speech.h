#pragma once


class Speech
{
public:

    Speech( const boost::program_options::variables_map& vm );
    ~Speech();
    void play( const std::string& word );
    void play( const std::vector<std::string>& words );
    std::vector<std::string> get_files( const std::string& word );
    std::vector<std::string> get_files( const std::vector<std::string>& words );
    void update_option( const boost::program_options::variables_map& vm );

public:

    std::vector<boost::filesystem::path> m_paths;
    std::vector<std::string> m_speech_path_option;
    boost::program_options::variables_map m_variables_map;
    boost::signals2::connection m_connection;
};
