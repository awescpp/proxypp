/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#define BOOST_TEST_MODULE test_rule_op

#include "proxypp/rule/op.h"
#include <boost/test/unit_test.hpp>
#include <vector>

namespace proxypp::rule ::test
{
  BOOST_AUTO_TEST_CASE(value_to_should_return_expected_value)
  {
    const std::vector<std::pair<std::string_view, Op>> cases {
      { "set", Op::Set },
      { "add", Op::Add },
      { "remove", Op::Remove },
      { "replace", Op::Replace },
    };

    for(const auto& [value, expected] : cases)
      {
        BOOST_TEST_CONTEXT("value is " << value)
        {
          BOOST_TEST(proxypp::json(value).as<Op>() == expected);
        }
      }
  }

  BOOST_AUTO_TEST_CASE(value_to_should_fail_when_value_is_invalid)
  {
    const std::vector<std::string_view> values { "unknown", "Set", "Add" };
    for(const auto& value : values)
      {
        BOOST_TEST_CONTEXT("value is " << value)
        {
          BOOST_CHECK_THROW(proxypp::json(value).as<Op>(), std::exception);
        }
      }
  }
}
