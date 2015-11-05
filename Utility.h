#pragma once
#ifdef max
#undef max
#undef min
#endif


typedef std::vector<std::time_t> time_list;
typedef std::map<size_t, time_list> history_type;


namespace Utility
{
    std::string time_string( std::time_t t, const char* format = "%Y/%m/%d %H:%M:%S" );
    std::string time_duration_string( std::time_t t );
    bool is_today( std::time_t t );
    std::string get_time_list_string( const time_list& times );
    std::string get_history_string( const history_type& history );
    std::ostream& output_time_list( std::ostream& os, const time_list& times );
    std::ostream& output_history( std::ostream& os, const history_type& history );
    std::vector<std::string> extract_words( const std::string& s );
    void play_sound( const std::string& file );
    void play_sound_list( const std::vector<std::string>& files );
    void play_sound_list_thread( const std::vector<std::string>& files );
    void text_to_speech( const std::string& word );
    void text_to_speech_list( const std::vector<std::string>& words );
    void text_to_speech_list_thread( const std::vector<std::string>& words );
    void play_or_tts( const std::pair<std::string, std::string>& word_path );
    void play_or_tts_list( const std::vector< std::pair<std::string, std::string> >& word_path_list );
    void play_or_tts_list_thread( const std::vector< std::pair<std::string, std::string> >& word_path_list );

    struct RecordSound
    {
        RecordSound( const std::string& n );
        ~RecordSound();
        std::string m_file_name;
    };

    size_t random_number( size_t lo = std::numeric_limits <size_t>::min(), size_t hi = std::numeric_limits <size_t>::max() );
}
