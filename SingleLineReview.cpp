#include "stdafx.h"
#include "SingleLineReview.h"


SingleLineReview::SingleLineReview( const std::string& file_name, const boost::program_options::variables_map& vm )
    : m_file_name( file_name ),
      m_variable_map( vm ),
      m_is_reviewing( false )
{
    std::srand ( unsigned ( std::time(0) ) );
    m_log_debug.add_attribute( "Level", boost::log::attributes::constant<std::string>( "DEBUG" ) );
    m_log_trace.add_attribute( "Level", boost::log::attributes::constant<std::string>( "TRACE" ) );
    m_log_test.add_attribute( "Level", boost::log::attributes::constant<std::string>( "TEST" ) );
    m_review_name   = boost::filesystem::change_extension( m_file_name, ".review" ).string();
    m_history_name  = boost::filesystem::change_extension( m_file_name, ".history" ).string();
    const std::time_t span[] =
    {
        0*minute,   1*minute,   3*minute,   7*minute,   15*minute,  30*minute,  30*minute,
        1*hour,     3*hour,     7*hour,     7*hour,     7*hour,     7*hour,     7*hour,
        1*day,      2*day,      3*day,      4*day,      5*day,      6*day,      7*day,
        7*day,      7*day,      7*day,      7*day,      7*day,      7*day,      7*day,
        1*month,    1*month,    1*month,    1*month,    1*month,    1*month,    1*month
    };
    m_review_spans.assign( span, span + sizeof(span) / sizeof(std::time_t) );
    new boost::thread( boost::bind( &SingleLineReview::collect_strings_to_review_thread, this ) );
}


void SingleLineReview::review()
{
    initialize_history();

    for ( ;; )
    {
        reload_strings();
        synchronize_history();
        collect_strings_to_review();
        wait_for_input( "\t********** " + boost::lexical_cast<std::string>(m_reviewing_strings.size() ) + " **********\n" );

        if ( ! m_reviewing_strings.empty() )
        {
            on_review_begin();

            for ( size_t i = 0; i < m_reviewing_strings.size(); ++i )
            {
                display_reviewing_string( m_reviewing_strings, i );

                std::string s = wait_for_input();

                if ( s == "repeat" || s == "r" )
                {
                    i--;
                    continue;
                }

                if ( s == "back" || s == "b" )
                {
                    if ( 0 < i ) i -= 2;
                    else if ( 0 == i ) i--;
                    continue;
                }
                
                if ( s == "skip" || s == "s" )
                {
                    continue;
                }

                write_review_time( m_reviewing_strings[i] );
            }

            on_review_end();
        }

        write_review_to_history();
    }
}


void SingleLineReview::on_review_begin()
{
    m_is_reviewing = true;
    m_review_strm.open( m_review_name.c_str(), std::ios::app );
    std::random_shuffle( m_reviewing_strings.begin(), m_reviewing_strings.end(), random_gen );
    system( ( "TITLE " + m_file_name + " - " + boost::lexical_cast<std::string>( m_reviewing_strings.size() ) ).c_str() );
}


void SingleLineReview::on_review_end()
{
    m_is_reviewing = false;
    m_review_strm.close();
    m_reviewing_strings.clear();
    system( ( "TITLE " + m_file_name ).c_str() );
}


void SingleLineReview::initialize_history()
{
    m_history.clear();
    bool should_write_history = false;
    history_type history = load_history_from_file( m_history_name );

    merge_history( history );
    
    if ( m_history != history )
    {
        BOOST_LOG(m_log) << __FUNCTION__ << " - wrong history detected.";
        should_write_history = true;
    }

    history_type review_history = load_history_from_file( m_review_name );

    if ( ! review_history.empty() )
    {
        BOOST_LOG(m_log) << __FUNCTION__ << " - review detected.";
        merge_history( review_history );
        boost::filesystem::remove( m_review_name );
        should_write_history = true;
    }

    if ( should_write_history )
    {
        write_history();
    }

    BOOST_LOG(m_log_debug) << __FUNCTION__ << " - history is uptodate.\n" << get_history_string( m_history );
}


bool SingleLineReview::reload_strings( hash_type hasher )
{
    static std::time_t m_last_write_time = 0;
    std::time_t t = boost::filesystem::last_write_time( m_file_name );

    if ( t == m_last_write_time )
    {
        return true;
    }

    m_last_write_time = t;

    std::ifstream is( m_file_name.c_str() );

    if ( !is )
    {
        BOOST_LOG(m_log) << __FUNCTION__ << " - cannot open file: " << m_file_name;
        return false;
    }

    m_strings.clear();

    for ( std::string s; std::getline( is, s ); )
    {
        boost::trim(s);

        if ( ! s.empty() )
        {
            if ( '#' == s[0] )
            {
                continue;
            }

            m_strings.push_back( std::make_pair( s, hasher(s) ) );
        }
    }

    return true;
}


void SingleLineReview::collect_strings_to_review()
{
    boost::unique_lock<boost::mutex> guard( m_mutex );
    m_reviewing_strings.clear();
    
    for ( size_t i = 0; i < m_strings.size(); ++i )
    {
        size_t hash = m_strings[i].second;
        time_list& times = m_history[hash];

        if ( times.empty() )
        {
            m_reviewing_strings.push_back( m_strings[i] );
            continue;
        }

        size_t review_round = times.size();

        if ( m_review_spans.size() == review_round )
        {
            BOOST_LOG(m_log) << __FUNCTION__ << " - finished(" << hash << "): " << m_strings[i].first;
            continue;
        }

        std::time_t current_time = std::time(0);
        std::time_t last_review_time = times.back();
        std::time_t span = m_review_spans[review_round];

        if ( last_review_time + span < current_time )
        {
            m_reviewing_strings.push_back( m_strings[i] );
        }

        if ( current_time < last_review_time )
        {
            BOOST_LOG(m_log) << __FUNCTION__ << " - wrong time: last-review-time=" << time_string( last_review_time );
        }

        BOOST_LOG(m_log_trace) << __FUNCTION__ << " -"
            << " first-review-time = " << time_string( times.front() )
            << " last-review-time = " << time_string(last_review_time)
            << " review-round = " << review_round
            << " span = " << time_duration_string( span )
            << " elapsed = " << time_duration_string( current_time - last_review_time )
            ;
    }
}


void SingleLineReview::collect_strings_to_review_thread()
{
    while ( true )
    {
        boost::this_thread::sleep_for( boost::chrono::seconds(30) );

        if ( false == m_is_reviewing )
        {
            collect_strings_to_review();

            if ( ! m_reviewing_strings.empty() )
            {
                system( ( "TITLE " + m_file_name + " - " + boost::lexical_cast<std::string>( m_reviewing_strings.size() ) ).c_str() ); 
            }
        }
    }
}


void SingleLineReview::write_review_time( const std::pair<std::string, size_t>& s )
{
    if ( ! m_review_strm )
    {
        BOOST_LOG(m_log) << __FUNCTION__ << " - cannot open for append: " << m_review_name;
        return;
    }

    m_review_strm << s.second << "\t" << std::time(0) << std::endl;

    if ( m_review_strm.fail() )
    {
        BOOST_LOG(m_log) << __FUNCTION__ << " - failed.";
    }
}


std::string SingleLineReview::wait_for_input( const std::string& message )
{
    if ( ! message.empty() )
    {
        std::cout << message << std::flush;
    }

    std::string input;
    std::getline( std::cin, input );
    system( "CLS" );
    boost::trim(input);
    return input;
}


void SingleLineReview::write_review_to_history()
{
    history_type review_history = load_history_from_file( m_review_name );

    if ( ! review_history.empty() )
    {
        merge_history( review_history );

        if ( write_history() )
        {
            boost::filesystem::remove( m_review_name );
        }
    }
}


void SingleLineReview::synchronize_history()
{
    std::set<size_t> hashes;

    for ( size_t i = 0; i < m_strings.size(); ++i )
    {
        hashes.insert( m_strings[i].second );
    }

    for ( history_type::iterator it = m_history.begin(); it != m_history.end(); NULL )
    {
        if ( hashes.find( it->first ) == hashes.end() )
        {
            m_history.erase( it++ );
        }
        else
        {
            ++it;
        }
    }
}


void SingleLineReview::display_reviewing_string( const std::vector< std::pair<std::string, size_t> >& strings, size_t index )
{
    const std::string& s = strings[index].first;

    if ( boost::starts_with( s, "[Q]" ) )
    {
        size_t pos = s.find( "[A]" );

        if ( pos == std::string::npos )
        {
            BOOST_LOG(m_log) << __FUNCTION__ << " - bad format: " << s;
            return;
        }

        std::string question = s.substr( 4, pos - 4 );
        std::string answer = s.substr( pos + 4 );
        boost::trim(question);
        boost::trim(answer);

        if ( !question.empty() && ! answer.empty() )
        {
            std::cout << "\t" << question << std::flush;
            std::getline( std::cin, std::string() );
            std::cout << "\t" << answer;
        }
    }
    else
    {
        std::cout << "\t" << strings[index].first;
    }

    std::cout << " [" << index + 1 << "/" << strings.size() << "] " << std::flush;
}


std::string SingleLineReview::time_string( std::time_t t, const char* format )
{
    std::tm* m = std::localtime( &t );
    char s[100] = { 0 };
    std::strftime( s, 100, format, m );
    return s;
}


std::string SingleLineReview::time_duration_string( std::time_t t )
{
    std::time_t ori = t;
    std::time_t mon = 0;
    std::time_t d = 0;
    std::time_t h = 0;
    std::time_t min = 0;

    if ( month <= t )
    {
        mon = t / month;
        t %= month;
    }

    if ( day <= t )
    {
        d = t / day;
        t %= day;
    }

    if ( hour <= t )
    {
        h = t / hour;
        t %= hour;
    }

    if ( minute <= t )
    {
        min =  t / minute;
    }

    std::stringstream strm;
    #define WRAP_ZERO(x) (9 < x ? "" : "0") << x 

    if ( mon || d )
    {
        strm << WRAP_ZERO(mon) << "/" << WRAP_ZERO(d) << "-";
    }

    strm << WRAP_ZERO(h) << ":" << WRAP_ZERO(min);
    #undef WRAP_ZERO

    BOOST_LOG(m_log_trace) << __FUNCTION__ << " - " << ori << " = " << strm.str();
    return strm.str();    
}


bool SingleLineReview::write_history()
{
    std::ofstream os( m_history_name.c_str() );

    if ( ! os )
    {
        BOOST_LOG(m_log) << __FUNCTION__ << " - can not open file for write " << m_history_name;
        return false;
    }

    for ( history_type::const_iterator it = m_history.begin(); it != m_history.end(); ++it )
    {
        os << it->first << " ";
        std::copy( it->second.begin(), it->second.end(), std::ostream_iterator<std::time_t>(os, " ") );
        os << std::endl;
    }

    return true;
}


history_type SingleLineReview::load_history_from_file( const std::string& file_name )
{
    history_type history;

    if ( ! boost::filesystem::exists( file_name ) )
    {
        return history;
    }

    std::ifstream is( file_name.c_str() );

    if ( ! is )
    {
        BOOST_LOG(m_log) << __FUNCTION__ << " - can not open file " << file_name;
        return history;
    }

    BOOST_LOG(m_log_trace) << __FUNCTION__ << " - " << file_name << " begin";

    size_t hash = 0;
    std::time_t time = 0;
    std::stringstream strm;

    for ( std::string s; std::getline( is, s ); )
    {
        BOOST_LOG(m_log_trace) << __FUNCTION__ << " - " << s;

        if ( ! s.empty() )
        {
            strm.clear();
            strm.str(s);
            strm >> hash;
            std::vector<std::time_t>& times = history[hash];

            while ( strm >> time )
            {
                times.push_back( time );
            }
        }
    }

    BOOST_LOG(m_log_trace) << __FUNCTION__ << " - " << file_name << " end";
    return history;
}


void SingleLineReview::merge_history( const history_type& history )
{
    for ( history_type::const_iterator it = history.begin(); it != history.end(); ++it )
    {
        size_t hash = it->first;
        const std::vector<std::time_t>& times = it->second;
        std::vector<std::time_t>& history_times = m_history[hash];
        size_t round = history_times.size();
        std::time_t last_time = ( round ? history_times.back() : 0 );

        for ( size_t i = 0; i < times.size(); ++i )
        {
            if ( last_time + m_review_spans[round] < times[i] )
            {
                history_times.push_back( times[i] );
                last_time = times[i];
                round++;
            }
        }
    }
}


std::ostream& SingleLineReview::output_history( std::ostream& os, const history_type& history )
{
    for ( history_type::const_iterator it = history.begin(); it != history.end(); ++it )
    {
        size_t hash = it->first;
        const time_list& times = it->second;
        os << hash;

        if ( times.empty() )
        {
            os << std::endl;
            continue;
        }

        os << " " << time_string( times[0] );

        for ( size_t i = 1; i < times.size(); ++i )
        {
            os << " " << time_duration_string( times[i] - times[i - 1] );
        }

        os << std::endl;
    }

    return os;
}


std::string SingleLineReview::get_history_string( const history_type& history )
{
    std::stringstream strm;
    output_history( strm, history );
    return strm.str();
}


std::ostream& SingleLineReview::output_time_list( std::ostream& os, const time_list& times )
{
    if ( times.empty() )
    {
        return os;
    }

    os << " " << time_string( times[0] );

    for ( size_t i = 1; i < times.size(); ++i )
    {
        os << " " << time_duration_string( times[i] - times[i - 1] );
    }

    return os;
}

std::string SingleLineReview::get_time_list_string( const time_list& times )
{
    std::stringstream strm;
    output_time_list( strm, times );
    return strm.str();
}


size_t SingleLineReview::string_hash( const std::string& str )
{
    std::string s = str;
    const char* chinese_chars[] =
    {
        "��", "��", "��", "��", "��", "��", "��", "��", "��", "��", "��", "��", "��", "��", "��", "@", "��", "��",
        "��", "��", "��", "��", "��", "��", "��", "��", "��", "��", "��", "��", "��", "��", "��", "��",
        "��", "����",  "��", "����"
    };

    for ( size_t i = 0; i < sizeof(chinese_chars) / sizeof(char*); ++i )
    {
        boost::erase_all( s, chinese_chars[i] );
    }

    s.erase( std::remove_if( s.begin(), s.end(), boost::is_any_of( " \t\"\',.?:;!-/#()|<>{}[]~`@$%^&*+" ) ), s.end() );
    boost::to_lower(s);
    static boost::hash<std::string> string_hasher;
    return string_hasher(s);
}


void SingleLineReview::update_hash_algorighom( hash_type old_hasher, hash_type new_hasher )
{
    // 1) create new_hash function
    // 2) use new hash here
    // 3) execute this
    // 4) replace ols hash to new hash

    initialize_history();
    reload_strings( old_hasher );

    history_type history;

    for ( size_t i = 0; i < m_strings.size(); ++i )
    {
        std::string s = m_strings[i].first;
        size_t hash = m_strings[i].second;
        time_list& times = m_history[hash];

        size_t new_hash = new_hasher(s);

        m_strings[i].second = new_hash;
        history[new_hash] = times;
    }

    if ( history.size() == m_history.size() )
    {
        m_history = history;
        write_history();
        BOOST_LOG(m_log_debug) << __FUNCTION__ << " - history updated";
    }
}
