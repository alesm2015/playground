#define BOOST_TEST_MODULE booking
#include <boost/test/unit_test.hpp>

#include "booking.h"

/*
    https://www.boost.org/doc/libs/1_85_0/libs/test/doc/html/boost_test/tests_organization/test_tree/test_suite.html
*/


BOOST_AUTO_TEST_SUITE(booking_suite)

BOOST_AUTO_TEST_CASE(booking_basic_case)
{
    BOOST_TEST_WARN( sizeof(int) < 4U );
}

BOOST_AUTO_TEST_SUITE_END()


