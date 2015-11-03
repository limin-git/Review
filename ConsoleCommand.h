#pragma once


class ConsoleCommand
{
public:

    ConsoleCommand();
    void update_option( const boost::program_options::variables_map& vm );

public:

    std::vector<std::string> m_commands;
};
