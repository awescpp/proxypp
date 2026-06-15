/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#define BOOST_TEST_MODULE BoostUrlTest

#include <boost/test/unit_test.hpp>
#include <boost/url.hpp>

BOOST_AUTO_TEST_SUITE(boost_url_test_suite)

BOOST_AUTO_TEST_CASE(parse_absolute_uri_without_scheme_should_fail)
{
  const auto url = "httpbin.org";
  const auto result = boost::urls::parse_absolute_uri(url);
  BOOST_TEST(result.has_error());
}

BOOST_AUTO_TEST_CASE(parse_absolute_uri_with_port_should_return_port)
{
  const auto url = "http://httpbin.org:3000";
  const auto result = boost::urls::parse_absolute_uri(url);
  BOOST_TEST(!result.has_error());
  BOOST_TEST(result->port() == "3000");
}

BOOST_AUTO_TEST_CASE(parse_absolute_uri_without_port_should_return_empty_port)
{
  const auto url = "http://httpbin.org";
  const auto result = boost::urls::parse_absolute_uri(url);
  BOOST_TEST(!result.has_error());
  BOOST_TEST(result->port() == "");
}

BOOST_AUTO_TEST_SUITE_END()