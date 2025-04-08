#define BOOST_TEST_MODULE tests
#include <boost/test/unit_test.hpp>


#include "booker.h"
#include "booking.h"

/*
    https://live.boost.org/doc/libs/1_87_0/libs/test/doc/html/boost_test/utf_reference.html
*/


BOOST_AUTO_TEST_SUITE(booking_suite)

CBooking booking_test;
CBooker::booker_ptr first_booker = std::make_shared<CBooker>();
CBooker::booker_ptr second_booker = std::make_shared<CBooker>();


/// @brief First part of test scope in booke. Set and join the booker
/// @param  booking_basic_test_case_1
BOOST_AUTO_TEST_CASE(booking_basic_test_case_1)
{
    int32_t rc;
    std::string str;

    BOOST_REQUIRE(first_booker);
    BOOST_REQUIRE(second_booker);

    BOOST_TEST_CHECKPOINT("When new booker is created, uid is empty");
    str = first_booker->get_booker_uid();
    BOOST_TEST(str.empty());

    BOOST_TEST_CHECKPOINT("Set booker uid");
    first_booker->set_uid("uid");
    str = first_booker->get_booker_uid();
    BOOST_CHECK_EQUAL(str, "uid");

    BOOST_TEST_CHECKPOINT("Proper response if booker is null");
    rc = booking_test.join_booker(nullptr);
    BOOST_CHECK_EQUAL(rc, -EINVAL);

    BOOST_TEST_CHECKPOINT("Proper response if booker is not null");
    rc = booking_test.join_booker(first_booker);
    BOOST_CHECK_EQUAL(rc, 1);
}

/// @brief Convert JSON to boost tree
/// @param  bookig_basic_test_case_2
BOOST_AUTO_TEST_CASE(bookig_basic_test_case_2)
{
    int32_t rc;
    std::string data;
    std::stringstream ss;;

    /*load dummy data*/
    boost::property_tree::ptree pt;

    BOOST_TEST_CHECKPOINT("Convert JSON to boost tree");
    data =\
        "{"\
        "\"movies\": ["\
        "               {"\
        "                   \"movie\": \"GodFather\","
        "                   \"theatres\": ["\
        "                           \"Tokyo\","\
        "                           \"Delhi\","\
        "                           \"Shanghai\","\
        "                           \"SaoPaulo\","\
        "                           \"MexicoCity\""\
        "                               ]"\
        "               }"\
        "            ]"\
        "}";

    ss << data;
    BOOST_CHECK_NO_THROW(boost::property_tree::read_json(ss, pt));

    rc = booking_test.load_data(pt);
    BOOST_CHECK_GE(rc, EXIT_SUCCESS);
}

/// @brief Test if booking & unbooking of seats works
/// @param  bookig_basic_test_case_3
BOOST_AUTO_TEST_CASE(bookig_basic_test_case_3)
{
    int32_t rc;
    std::string str1;
    std::string str2;
    std::set<uint32_t> set;
    std::set<uint32_t> set2;
    std::vector<uint32_t> unavalable_seats;
    std::vector<uint32_t> tmpv;

    str1 = "GodFather";
    str2 = "Delhi";

    BOOST_TEST_CHECKPOINT("Try to book seat, which doesn't exist");
    set = std::set<uint32_t>({22});
    rc = booking_test.book_seats(first_booker, str1, str2, set, unavalable_seats, false);
    BOOST_CHECK_LT(rc, EXIT_SUCCESS);
    BOOST_CHECK(unavalable_seats.empty());

    BOOST_TEST_CHECKPOINT("Try to book seat, movie, which doesnt exist");
    str1 = "aa";
    set = std::set<uint32_t>({15});
    rc = booking_test.book_seats(first_booker, str1, str2, set, unavalable_seats, false);
    BOOST_CHECK_LT(rc, EXIT_SUCCESS);
    BOOST_CHECK(unavalable_seats.empty());

    BOOST_TEST_CHECKPOINT("Try to book seat, theatre, which doesnt exist");
    str1 = "GodFather";
    str2 = "Delhi2";
    rc = booking_test.book_seats(first_booker, str1, str2, set, unavalable_seats, false);
    BOOST_CHECK_LT(rc, EXIT_SUCCESS);
    BOOST_CHECK(unavalable_seats.empty());

    BOOST_TEST_CHECKPOINT("Book a seats");
    str2 = "Delhi";
    set = std::set<uint32_t>({17, 12});
    rc = booking_test.book_seats(first_booker, str1, str2, set, unavalable_seats, false);
    BOOST_CHECK_EQUAL(rc, 2);
    BOOST_CHECK(unavalable_seats.empty());

    BOOST_TEST_CHECKPOINT("Check booked seats");
    rc = booking_test.get_booked_seats(first_booker, str1, str2, set2);
    BOOST_CHECK_EQUAL(rc, 2);
    BOOST_CHECK_EQUAL_COLLECTIONS(set.begin(), set.end(), set2.begin(), set2.end());
    BOOST_CHECK(unavalable_seats.empty());

    BOOST_TEST_CHECKPOINT("Re-Book same seats");
    str2 = "Delhi";
    set = std::set<uint32_t>({17});
    rc = booking_test.book_seats(first_booker, str1, str2, set, unavalable_seats, false);
    BOOST_CHECK_EQUAL(rc, 2);

    BOOST_TEST_CHECKPOINT("Get free seats, movie doesn't exist");
    str1 = "aa";
    set.clear();
    rc = booking_test.get_free_seats(str1, str2, set);
    BOOST_CHECK_LT(rc, EXIT_SUCCESS);
 
    BOOST_TEST_CHECKPOINT("Get free seats, theatre doesn't exist");
    str1 = "GodFather";
    str2 = "Delhi2";
    rc = booking_test.get_free_seats(str1, str2, set);
    BOOST_CHECK_LT(rc, EXIT_SUCCESS);

    BOOST_TEST_CHECKPOINT("Get free seats");
    set.clear();
    set2.clear();
    for (uint32_t i = 0; i < booking_test.get_max_seats(); ++i) {
        set2.insert(i);
    }
    set2.erase(12);
    set2.erase(17);

    str2 = "Delhi";
    rc = booking_test.get_free_seats(str1, str2, set);
    BOOST_CHECK_GE(rc, EXIT_SUCCESS);
    BOOST_CHECK_EQUAL_COLLECTIONS(set.begin(), set.end(), set2.begin(), set2.end());

    BOOST_TEST_CHECKPOINT("Unbook seat - wrong movie");
    str1 = "aa";
    rc = booking_test.unbook_seats(first_booker, str1, str2, set, unavalable_seats);
    BOOST_CHECK_LT(rc, EXIT_SUCCESS);

    BOOST_TEST_CHECKPOINT("Unbook seat - wrong theatre");
    str1 = "GodFather";
    str2 = "Delhi2";
    rc = booking_test.unbook_seats(first_booker, str1, str2, set, unavalable_seats);
    BOOST_CHECK_LT(rc, EXIT_SUCCESS);

    BOOST_TEST_CHECKPOINT("Unbook seat - seat doesn't exist");
    str2 = "Delhi";
    set = std::set<uint32_t>({22});
    rc = booking_test.unbook_seats(first_booker, str1, str2, set, unavalable_seats);
    BOOST_CHECK_LT(rc, EXIT_SUCCESS);
    BOOST_CHECK(unavalable_seats.empty());

    BOOST_TEST_CHECKPOINT("Unbook seat - seat is free");
    str2 = "Delhi";
    set = std::set<uint32_t>({10});
    tmpv = std::vector<uint32_t>({10});
    rc = booking_test.unbook_seats(first_booker, str1, str2, set, unavalable_seats);
    BOOST_CHECK_GE(rc, EXIT_SUCCESS);
    BOOST_CHECK_EQUAL_COLLECTIONS(tmpv.begin(), tmpv.end(), unavalable_seats.begin(), unavalable_seats.end());

    BOOST_TEST_CHECKPOINT("Unbook seat - invalid booker");
    str2 = "Delhi";
    set = std::set<uint32_t>({17});
    tmpv = std::vector<uint32_t>({17});
    rc = booking_test.unbook_seats(second_booker, str1, str2, set, unavalable_seats);
    BOOST_CHECK_EQUAL(rc, 1);
    BOOST_CHECK_EQUAL_COLLECTIONS(tmpv.begin(), tmpv.end(), unavalable_seats.begin(), unavalable_seats.end());

    BOOST_TEST_CHECKPOINT("Unbook seat");
    str2 = "Delhi";
    set = std::set<uint32_t>({17});
    rc = booking_test.unbook_seats(first_booker, str1, str2, set, unavalable_seats);
    BOOST_CHECK_EQUAL(rc, 1);
    BOOST_CHECK(unavalable_seats.empty());

    BOOST_TEST_CHECKPOINT("Get free seats - 2");
    set.clear();
    set2.clear();
    for (uint32_t i = 0; i < booking_test.get_max_seats(); ++i) {
        set2.insert(i);
    }
    set2.erase(12);
    rc = booking_test.get_free_seats(str1, str2, set);
    BOOST_CHECK_GE(rc, EXIT_SUCCESS);
    BOOST_CHECK_EQUAL_COLLECTIONS(set.begin(), set.end(), set2.begin(), set2.end());

    BOOST_TEST_CHECKPOINT("Book a new seats");
    str2 = "Delhi";
    set = std::set<uint32_t>({8, 9, 10});
    rc = booking_test.book_seats(first_booker, str1, str2, set, unavalable_seats, false);
    BOOST_CHECK_EQUAL(rc, 4);
    BOOST_CHECK(unavalable_seats.empty());

    BOOST_TEST_CHECKPOINT("Check booked seats");
    set = std::set<uint32_t>({8, 9, 10, 12});
    rc = booking_test.get_booked_seats(first_booker, str1, str2, set2);
    BOOST_CHECK_EQUAL(rc, 4);
    BOOST_CHECK_EQUAL_COLLECTIONS(set.begin(), set.end(), set2.begin(), set2.end());

    BOOST_TEST_CHECKPOINT("Get free seats - 2");
    set.clear();
    set2.clear();
    for (uint32_t i = 0; i < booking_test.get_max_seats(); ++i) {
        set2.insert(i);
    }
    set2.erase(8);
    set2.erase(9);
    set2.erase(10);
    set2.erase(12);
    rc = booking_test.get_free_seats(str1, str2, set);
    BOOST_CHECK_GE(rc, EXIT_SUCCESS);
    BOOST_CHECK_EQUAL_COLLECTIONS(set.begin(), set.end(), set2.begin(), set2.end());

    BOOST_TEST_CHECKPOINT("Check booked seats - 3");
    set2.clear();
    rc = booking_test.get_booked_seats(second_booker, str1, str2, set2);
    BOOST_CHECK_EQUAL(rc, 0);
    BOOST_CHECK(set2.empty());

    BOOST_TEST_CHECKPOINT("Book a new seats - one is already booked");
    str2 = "Delhi";
    set = std::set<uint32_t>({10, 15});
    tmpv = std::vector<uint32_t>({10});
    rc = booking_test.book_seats(second_booker, str1, str2, set, unavalable_seats, false);
    BOOST_CHECK_GE(rc, EXIT_SUCCESS);
    BOOST_CHECK_EQUAL_COLLECTIONS(tmpv.begin(), tmpv.end(), unavalable_seats.begin(), unavalable_seats.end());

    BOOST_TEST_CHECKPOINT("Book a new seats - best efford");
    str2 = "Delhi";
    set = std::set<uint32_t>({10, 15});
    tmpv = std::vector<uint32_t>({10});
    rc = booking_test.book_seats(second_booker, str1, str2, set, unavalable_seats, true);
    BOOST_CHECK_EQUAL(rc, 1);
    BOOST_CHECK_EQUAL_COLLECTIONS(tmpv.begin(), tmpv.end(), unavalable_seats.begin(), unavalable_seats.end());

    BOOST_TEST_CHECKPOINT("Check booked seats - 4");
    set2.clear();
    set = std::set<uint32_t>({15});
    rc = booking_test.get_booked_seats(second_booker, str1, str2, set2);
    BOOST_CHECK_EQUAL(rc, 1);
    BOOST_CHECK_EQUAL_COLLECTIONS(set.begin(), set.end(), set2.begin(), set2.end());

    BOOST_TEST_CHECKPOINT("Get free seats - 3");
    set.clear();
    set2.clear();
    for (uint32_t i = 0; i < booking_test.get_max_seats(); ++i) {
        set2.insert(i);
    }
    set2.erase(8);
    set2.erase(9);
    set2.erase(10);
    set2.erase(12);
    set2.erase(15);
    rc = booking_test.get_free_seats(str1, str2, set);
    BOOST_CHECK_GE(rc, EXIT_SUCCESS);
    BOOST_CHECK_EQUAL_COLLECTIONS(set.begin(), set.end(), set2.begin(), set2.end());
}

BOOST_AUTO_TEST_SUITE_END()


