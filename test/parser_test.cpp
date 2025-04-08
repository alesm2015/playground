#include <boost/test/unit_test.hpp>


#include "parser.h"


/*
    https://live.boost.org/doc/libs/1_87_0/libs/test/doc/html/boost_test/utf_reference.html
*/


BOOST_AUTO_TEST_SUITE(parser_suite)

/// @brief First part of test scope in booke. Set and join the booker
/// @param  parser_test_case_1
BOOST_AUTO_TEST_CASE(parser_test_case_1)
{
    std::set<uint32_t> set1;
    std::set<uint32_t> set2({5, 6, 8, 9, 10, 11, 12, 13, 14, 2});
    std::vector<uint32_t> vect({3, 4, 5});
    std::string str = "5, 6, 8, 9 - 14, 2";
    std::string str2 = "2, 5, 6, 8, 9, 10, 11, 12, 13, 14";

    BOOST_TEST_CHECKPOINT("Check is multiple seats function works");
    set1 = get_seats(str);
    BOOST_CHECK_EQUAL_COLLECTIONS(set1.begin(), set1.end(), set2.begin(), set2.end());

    BOOST_TEST_CHECKPOINT("Set to string");
    str.clear();
    seats_to_string(str, set1);
    BOOST_CHECK_EQUAL(str, str2);

    BOOST_TEST_CHECKPOINT("Vect to string");
    str.clear();
    str2 = "3, 4, 5";
    seats_to_string(str, vect);
    BOOST_CHECK_EQUAL(str, str2);
}


BOOST_AUTO_TEST_SUITE_END()
