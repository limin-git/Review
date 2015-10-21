#pragma once


class Speech
{
public:

    Speech( const boost::program_options::variables_map& vm );
    void play( const std::string& word );
    void play( const std::vector<std::string>& words );

public:

    std::vector<boost::filesystem::path> m_path;
    boost::program_options::variables_map m_variables_map;
};
