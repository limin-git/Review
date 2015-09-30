// Review.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Library/Utility.h"
#include "SingleLineReview.h"


std::ostream& save_review_time( std::ostream& os, size_t hash )
{
    return os << hash << "\t" << std::time(0) << std::endl;
}


//void test()
//{
//    std::ofstream os( "test.review", std::ios::app );
//
//    while ( true )
//    {
//        std::random_shuffle( vs.begin(), vs.end() );
//
//        for ( size_t i = 0; i < vs.size(); ++i )
//        {
//            std::cout << vs[i].first << ", " << vs[i].second;
//            save_review_time( os, vs[i].second );
//            system( "pause >NUL" );
//            std::cout << std::endl;
//        }
//
//        std::cout << "****** review again ******" << std::endl;
//        system( "pause >NUL" );
//        system( "cls" );
//    }
//}


int _tmain(int argc, _TCHAR* argv[])
{
    SingleLineReview review( "test.txt" );
    review.save_review_to_history( 0 );
    review.initialize();
    review.review();

	return 0;
}
