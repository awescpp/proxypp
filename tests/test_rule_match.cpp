/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#define BOOST_TEST_MODULE test_rule_match_struct

#include <boost/test/unit_test.hpp>
#include <proxypp/rule/match.h>

namespace proxypp::rule::test
{
  BOOST_AUTO_TEST_CASE(value_to_should_return_match)
  {
    const auto content = R"JSON({
      "expr": "ctx.request.host === 'example.com'"
    })JSON";
    const auto value = proxypp::json::parse(content);
    const auto match = value.as<Match>();
    BOOST_TEST(match.expr == "ctx.request.host === 'example.com'");
  }

  // The following assertions are guaranteed by the JSON Schema, so they are
  // not tested here.
  // 1. value_to_should_fail_when_expr_is_not_a_string
  // 2. value_to_should_fail_when_expr_is_not_present

}