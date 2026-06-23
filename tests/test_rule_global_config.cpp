/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#define BOOST_TEST_MODULE rule_global_config_tests

#include "proxypp/rule/config.h"
#include <boost/json.hpp>
#include <boost/test/unit_test.hpp>
#include <exception>

namespace json = boost::json;

namespace proxypp::rule::test
{
  BOOST_AUTO_TEST_SUITE(RuleGlobalConfigTests)

  BOOST_AUTO_TEST_CASE(
    value_to_should_return_valid_config_object_when_only_version_was_given)
  {
    const auto value = json::parse(R"json({
      "version": 1
    })json");
    const auto config = json::value_to<rule::Config>(value);
    BOOST_TEST(!config.http.has_value());
    BOOST_TEST(!config.schema.has_value());
    BOOST_TEST(config.version == 1);
  }

  BOOST_AUTO_TEST_CASE(value_to_should_fail_when_version_is_not_present)
  {
    const auto value = json::parse(R"json({})json");
    BOOST_CHECK_THROW(json::value_to<rule::Config>(value), std::exception);
  }

  BOOST_AUTO_TEST_CASE(
    value_to_should_return_valid_config_object_when_json_object_is_valid)
  {
    const auto value = json::parse(R"json(
      {
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
                  "op": "header.set",
                  "name": "Authorization",
                  "value": "Basic dXNlcjpwYXNzd29yZA=="
                }
              ]
            }
          ]
        }
      }
    )json");

    const auto config = json::value_to<rule::Config>(value);
    BOOST_REQUIRE(config.schema.has_value());
    BOOST_TEST(*config.schema == "../../schemas/proxypp_rules_schema_v1.json");
    BOOST_TEST(config.version == 1);
    BOOST_REQUIRE(config.http.has_value());
    BOOST_REQUIRE(config.http->rules.has_value());
    BOOST_TEST(config.http->rules->size() == 1);
  }

  BOOST_AUTO_TEST_CASE(
    value_to_should_fail_when_json_object_missing_required_property)
  {
    const auto value = json::parse(R"json(
      {
        "version": 1,
        "http": {
          "rules": [
            {
              "name": "add basic authorization header for 192.168.52.101:3067",
              "phase": "request",
              "match": {},
              "actions": [
                {
                  "op": "header.remove",
                  "name": "Authorization"
                }
              ]
            }
          ]
        }
      }
    )json");

    BOOST_CHECK_THROW(json::value_to<rule::Config>(value), std::exception);
  }

  BOOST_AUTO_TEST_CASE(
    value_to_should_return_config_with_http_when_http_object_is_empty)
  {
    const auto value = json::parse(R"json({
      "version": 1,
      "http": {}
    })json");
    const auto config = json::value_to<rule::Config>(value);
    BOOST_TEST(config.version == 1);
    BOOST_REQUIRE(config.http.has_value());
    BOOST_TEST(!config.http->rules.has_value());
  }

  BOOST_AUTO_TEST_CASE(
    value_to_should_return_config_with_empty_http_rules_when_rules_is_empty_array)
  {
    const auto value = json::parse(R"json({
      "version": 1,
      "http": {
        "rules": []
      }
    })json");
    const auto config = json::value_to<rule::Config>(value);
    BOOST_TEST(config.version == 1);
    BOOST_REQUIRE(config.http.has_value());
    BOOST_REQUIRE(config.http->rules.has_value());
    BOOST_TEST(config.http->rules->empty());
  }

  BOOST_AUTO_TEST_CASE(value_to_should_fail_when_http_is_not_object)
  {
    const auto value = json::parse(R"json({
      "version": 1,
      "http": []
    })json");
    BOOST_CHECK_THROW(json::value_to<rule::Config>(value), std::exception);
  }

  BOOST_AUTO_TEST_CASE(value_to_should_fail_when_version_is_string)
  {
    const auto value = json::parse(R"json({
      "version": "1"
    })json");
    BOOST_CHECK_THROW(json::value_to<rule::Config>(value), std::exception);
  }

  BOOST_AUTO_TEST_CASE(value_to_should_fail_when_version_is_float)
  {
    const auto value = json::parse(R"json({
      "version": 1.0
    })json");
    BOOST_CHECK_THROW(json::value_to<rule::Config>(value), std::exception);
  }

  BOOST_AUTO_TEST_CASE(value_to_should_fail_when_schema_is_not_string)
  {
    const auto value = json::parse(R"json(
    {
      "$schema": 1,
      "version": 1
    }
  )json");

    BOOST_CHECK_THROW(json::value_to<rule::Config>(value), std::exception);
  }

  BOOST_AUTO_TEST_SUITE_END()
}