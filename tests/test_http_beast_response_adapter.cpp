/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#define BOOST_TEST_MODULE test_http_beast_response_adapter

#include "test_http_beast_adapter_helper.h"
#include <boost/test/unit_test.hpp>

namespace proxypp::http::adapter::test
{
  BOOST_AUTO_TEST_CASE(add_header_should_add_header_when_missing)
  {
    http_::response_header<> response_header;
    detail::add_header_should_add_header_when_missing(response_header);
  }

  BOOST_AUTO_TEST_CASE(add_header_should_not_override_existing_header)
  {
    http_::response_header<> response_header;
    detail::add_header_should_not_override_existing_header(response_header);
  }

  BOOST_AUTO_TEST_CASE(set_header_should_add_header_when_missing)
  {
    http_::response_header<> response_header;
    detail::set_header_should_add_header_when_missing(response_header);
  }

  BOOST_AUTO_TEST_CASE(set_header_should_override_existing_header)
  {
    http_::response_header<> response_header;
    detail::set_header_should_override_existing_header(response_header);
  }

  BOOST_AUTO_TEST_CASE(replace_header_should_replace_existing_header)
  {
    http_::response_header<> response_header;
    detail::replace_header_should_replace_existing_header(response_header);
  }

  BOOST_AUTO_TEST_CASE(replace_header_should_do_nothing_when_header_missing)
  {
    http_::response_header<> response_header;
    detail::replace_header_should_do_nothing_when_header_missing(
      response_header);
  }

  BOOST_AUTO_TEST_CASE(remove_header_should_remove_existing_header)
  {
    http_::response_header<> response_header;
    detail::remove_header_should_remove_existing_header(response_header);
  }

}
