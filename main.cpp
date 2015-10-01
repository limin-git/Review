// Review.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "SingleLineReview.h"


int _tmain(int argc, _TCHAR* argv[])
{
    SingleLineReview review( "test.txt" );
    review.save_review_to_history();
    review.initialize();
    review.review();

	return 0;
}
