/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#define BOOST_TEST_MODULE test_rule_config

#include <boost/test/unit_test.hpp>
#include <proxypp/rule/config.h>

namespace proxypp::rule::test
{

  // these cases are validated by JSON Schema:
  // 1. unknown top-level property
  // 2. missing version
  // 3. invalid version

  BOOST_AUTO_TEST_CASE(value_to_config_should_return_expected_with_only_version)
  {
    const auto content = R"JSON({
      "version": 1
    })JSON";
    const auto value = boost::json::parse(content);
    const auto config = boost::json::value_to<rule::Config>(value);
    BOOST_TEST(config.version == 1);
    BOOST_TEST(config.schema.has_value() == false);
    BOOST_TEST(config.http.has_value() == false);
  }

  BOOST_AUTO_TEST_CASE(value_to_config_should_return_expected)
  {
    const auto content = R"JSON({
        "$schema": "../../schemas/proxypp_rules_schema_v1.json",
        "version": 1,
        "http": {
          "rules": [
            {
              "name": "add basic authorization header for 192.168.52.101:3067",
              "phase": "request",
              "match": {
                "expr": "ctx.request.host==='192.168.52.101' && ctx.request.port===3067"
              },
              "actions": [
                {
                  "op": "set",
                  "target": "header",
                  "data": {
                    "name": "Authorization",
                    "value": "Basic dXNlcjpwYXNzd29yZA=="
                  }
                }
              ]
            }
          ]
        }
      })JSON";
    const auto value = boost::json::parse(content);
    const auto config = boost::json::value_to<rule::Config>(value);
    BOOST_TEST(config.version == 1);
    BOOST_REQUIRE(config.schema.has_value());
    BOOST_TEST(*config.schema == "../../schemas/proxypp_rules_schema_v1.json");
    BOOST_REQUIRE(config.http.has_value());
    BOOST_REQUIRE(config.http->rules.has_value());
    BOOST_TEST(config.http->rules->size() == 1);
  }
}
