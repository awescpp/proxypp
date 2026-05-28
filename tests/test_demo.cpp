#define BOOST_TEST_MODULE DemoTest

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(demo_test_suite)

BOOST_AUTO_TEST_CASE(operation_expected_result) { BOOST_TEST(1 + 1 == 2); }

BOOST_AUTO_TEST_SUITE_END()
