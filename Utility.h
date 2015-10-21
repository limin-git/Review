#pragma once
typedef std::vector<std::time_t> time_list;
typedef std::map<size_t, time_list> history_type;


namespace Utility
{
    std::string time_string( std::time_t t, const char* format = "%Y/%m/%d %H:%M:%S" );
    std::string time_duration_string( std::time_t t );
    std::string get_time_list_string( const time_list& times );
    std::string get_history_string( const history_type& history );
    std::ostream& output_time_list( std::ostream& os, const time_list& times );
    std::ostream& output_history( std::ostream& os, const history_type& history );
    void play_sound( const std::string& file );
    void play_sounds( const std::vector<std::string>& files );
    void play_sound_thread( const std::string& file );
    void play_sound_thread( const std::vector<std::string>& files );
}
