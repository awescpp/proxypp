/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#define BOOST_TEST_MODULE json_helper_tests

#include "proxypp/error.h"
#include "proxypp/helper/json_helper.h"
#include <boost/test/unit_test.hpp>

namespace proxypp::helper::json::test
{
  BOOST_AUTO_TEST_CASE(parse_json_should_return_value_when_content_is_valid)
  {
    const auto content = R"JSON(
{
  "$schema": "../../schemas/proxypp_rules_schema_v1.json",
  "version": 1,
  "http": {
    "rules": [
      {
        "name": "set user-agent for all outgoing requests",
        "phase": "request",
        "actions": [
          {
            "op": "set",
            "target": "header",
            "data": {
              "name": "User-Agent",
              "value": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36"
            }
          }
        ]
      }
    ]
  }
}
    )JSON";

    auto parse_result = ParseJson(content);
    BOOST_REQUIRE(parse_result.has_value());
    auto jv = *parse_result;
    BOOST_TEST(jv.at("$schema")
               == "../../schemas/proxypp_rules_schema_v1.json");
    BOOST_TEST(jv.at("version") == 1);
    BOOST_TEST(jv.contains("http"));
  }

  BOOST_AUTO_TEST_CASE(parse_json_should_return_error_when_content_is_invalid)
  {
    const auto content = R"JSON(
        {
          "name": "foobar",
          "value": 42,
        }
    )JSON";
    auto parse_result = ParseJson(content);
    BOOST_REQUIRE(!parse_result.has_value());
  }

  BOOST_AUTO_TEST_CASE(parse_json_should_keep_error_message_when_parse_failed)
  {
    const auto content = R"JSON(
        {
          "name": "foobar",
          "value": 42,
        }
    )JSON";
    auto parse_result = ParseJson(content);
    BOOST_REQUIRE(!parse_result.has_value());
    BOOST_TEST(parse_result.error().message().size() > 0);
    BOOST_TEST_MESSAGE(parse_result.error().message());
  }
}