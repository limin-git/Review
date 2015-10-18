#include "stdafx.h"
#include "Loader.h"
#include "Utility.h"


Loader::Loader( const boost::program_options::variables_map& vm )
    : m_variables_map( vm ),
      m_last_write_time( 0 )
{
    m_file_name = vm["file-name"].as<std::string>();

    m_log_debug.add_attribute( "Level", boost::log::attributes::constant<std::string>( "DEBUG" ) );
    m_log_trace.add_attribute( "Level", boost::log::attributes::constant<std::string>( "TRACE" ) );
    m_log_test.add_attribute( "Level", boost::log::attributes::constant<std::string>( "TEST" ) );
}


const std::set<size_t>& Loader::get_string_hash_set()
{
    reload();
    return m_string_hash_set;
}


const std::string& Loader::get_string( size_t hash )
{
    reload();

    std::map<size_t, std::string>::iterator it = m_hash_2_string_map.find( hash );

    if ( it != m_hash_2_string_map.end() )
    {
        return it->second;
    }

    static std::string empty;
    return empty;
}


void Loader::reload()
{
    std::time_t t = boost::filesystem::last_write_time( m_file_name );

    if ( t == m_last_write_time )
    {
        return;
    }

    BOOST_LOG(m_log_debug) << __FUNCTION__ << " - " << "last-writ-time: " << Utility::time_string( m_last_write_time ) << ", new last-write-time: " << Utility::time_string( t );
    m_last_write_time = t;

    std::ifstream is( m_file_name.c_str() );

    if ( !is )
    {
        BOOST_LOG(m_log) << __FUNCTION__ << " - cannot open file: " << m_file_name;
        return;
    }

    std::set<size_t> string_hash_set;
    std::map<size_t, std::string> hash_2_string_map;

    for ( std::string s; std::getline( is, s ); )
    {
        boost::trim(s);
        boost::replace_all( s, "\\n", "\n" );
        boost::replace_all( s, "\\t", "\t" );

        if ( s.empty() || '#' == s[0] )
        {
            continue;
        }

        size_t hash = string_hash( s );
        string_hash_set.insert( hash );
        hash_2_string_map[hash] = s;
    }

    if ( m_string_hash_set != string_hash_set )
    {
        BOOST_LOG(m_log_debug) << __FUNCTION__ << " - " << "old-size" << m_string_hash_set.size() << ", new-size = " << string_hash_set.size();
        m_string_hash_set = string_hash_set;
        m_hash_2_string_map = hash_2_string_map;
    }
}


size_t Loader::string_hash( const std::string& str )
{
    std::string s = str;
    const char* chinese_chars[] =
    {
        "¡¡", "£¬", "¡£", "¡¢", "£¿", "£¡", "£»", "£º", "¡¤", "£®", "¡°", "¡±", "¡®", "¡¯",
        "£à", "£­", "£½", "¡«", "£À", "££", "£¤", "£¥", "£ª", "£ß", "£«", "£ü", "¡ª", "¡ª¡ª",  "¡­", "¡­¡­",
        "¡¶", "¡·", "£¨", "£¨", "¡¾", "¡¿", "¡¸", "¡¹", "¡º", "¡»", "¡¼", "¡½", "¡´", "¡µ", "£û", "£ý",
        "\\n", "\\t", "\\nt"
    };

    for ( size_t i = 0; i < sizeof(chinese_chars) / sizeof(char*); ++i )
    {
        boost::erase_all( s, chinese_chars[i] );
    }

    s.erase( std::remove_if( s.begin(), s.end(), boost::is_any_of( " \t\"\',.?:;!-/#()|<>{}[]~`@$%^&*+\n\t" ) ), s.end() );
    boost::to_lower(s);
    static boost::hash<std::string> string_hasher;
    BOOST_LOG(m_log_debug) << __FUNCTION__ << " - " << "hash = " << string_hasher(s) << " \t" << s;
    return string_hasher(s);
}
