#pragma once


struct Log
{
    static void initialize( const boost::program_options::variables_map& vm = boost::program_options::variables_map() );
    static boost::program_options::options_description get_description();
};
