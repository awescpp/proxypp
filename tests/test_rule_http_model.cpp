/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#define BOOST_TEST_MODULE test_rule_http_models

#include <boost/test/unit_test.hpp>
#include <proxypp/rule/http/model.h>

// The goal of unit test suites in this file is to verify the normal parsing of
// JSON into related struct and enums. Invalid input is handled by the JSON
// Schema, so this file does not cover such cases.

namespace proxypp::rule::http
{
  namespace data::test
  {
    BOOST_AUTO_TEST_CASE(value_to_header_name_data_should_return_expected)
    {
      const auto content = R"JSON({
        "name": "X-PROXYPP-VER"
      })JSON";
      const auto value = boost::json::parse(content);
      const auto name_only_data = boost::json::value_to<HeaderNameData>(value);
      BOOST_TEST(name_only_data.name == "X-PROXYPP-VER");
    }

    BOOST_AUTO_TEST_CASE(value_to_header_name_value_data_should_return_expected)
    {
      const auto content = R"JSON({
        "name": "X-PROXYPP-VER",
        "value": "0.1.0"
      })JSON";
      const auto value = boost::json::parse(content);
      const auto name_value_data
        = boost::json::value_to<HeaderNameValueData>(value);
      BOOST_TEST(name_value_data.name == "X-PROXYPP-VER");
      BOOST_TEST(name_value_data.value == "0.1.0");
    }

  }

  namespace test
  {
    BOOST_AUTO_TEST_CASE(phase_from_string_should_return_expected)
    {
      BOOST_TEST(PhaseFromString("request") == Phase::Request);
      BOOST_TEST(PhaseFromString("response") == Phase::Response);
    }

    BOOST_AUTO_TEST_CASE(target_from_string_should_return_expected)
    {
      BOOST_TEST(TargetFromString("header") == Target::Header);
    }

    BOOST_AUTO_TEST_SUITE(ActionTests)

    BOOST_AUTO_TEST_CASE(
      value_to_action_with_name_value_data_should_return_expected)
    {
      const std::vector<std::pair<std::string_view, rule::Op>> cases {
        { "set", Op::Set },
        { "add", Op::Add },
        { "replace", Op::Replace },
      };

      for(const auto& [op, expected] : cases)
        {
          BOOST_TEST_CONTEXT("op is " << op)
          {
            const auto content = std::format(R"JSON({{
              "op": "{}",
              "target": "header",
              "data": {{
                "name": "Authorization",
                "value": "Basic..."
              }}
            }})JSON",
                                             op);

            const auto value = boost::json::parse(content);
            const auto action = boost::json::value_to<Action>(value);
            BOOST_TEST(action.op == expected);
            BOOST_TEST(action.target == Target::Header);
            BOOST_REQUIRE(
              std::holds_alternative<data::HeaderNameValueData>(action.data));
            const auto data = std::get<data::HeaderNameValueData>(action.data);
            BOOST_TEST(data.name == "Authorization");
            BOOST_TEST(data.value == "Basic...");
          }
        }
    }

    BOOST_AUTO_TEST_CASE(
      value_to_action_with_name_only_data_should_return_expected)
    {
      const auto content = R"JSON({
        "op": "remove",
        "target": "header",
        "data": {
          "name": "Authorization"
        }
      })JSON";
      const auto value = boost::json::parse(content);
      const auto action = boost::json::value_to<Action>(value);
      BOOST_TEST(action.op == rule::Op::Remove);
      BOOST_TEST(action.target == Target::Header);
      BOOST_REQUIRE(std::holds_alternative<data::HeaderNameData>(action.data));
      const auto data = std::get<data::HeaderNameData>(action.data);
      BOOST_TEST(data.name == "Authorization");
    }

    // value_to_action_should_fail_when_data_is_mismatch_with_op_and_target
    // will validated by JSON Schema

    BOOST_AUTO_TEST_SUITE_END()

    BOOST_AUTO_TEST_SUITE(RuleTests)

    BOOST_AUTO_TEST_CASE(value_to_rule_should_return_expected)
    {
      const auto content = R"JSON({
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
              "value": "Basic..."
            }
          }
        ]
      })JSON";
      const auto value = boost::json::parse(content);
      const auto rule = boost::json::value_to<Rule>(value);
      BOOST_TEST(rule.name
                 == "add basic authorization header for 192.168.52.101:3067");
      BOOST_TEST(rule.enabled == true);
      BOOST_TEST(rule.phase == Phase::Request);
      BOOST_REQUIRE(rule.match.has_value());
      BOOST_TEST(
        rule.match->expr
        == "ctx.request.host==='192.168.52.101' && ctx.request.port===3067");
      BOOST_TEST(rule.actions.size() == 1);
    }

    BOOST_AUTO_TEST_CASE(
      value_to_rule_should_return_disabled_rule_when_enabled_is_false)
    {
      const auto content = R"JSON({
        "name": "add basic authorization header for 192.168.52.101:3067",
        "phase": "request",
        "enabled": false,
        "match": {
          "expr": "ctx.request.host==='192.168.52.101' && ctx.request.port===3067"
        },
        "actions": [
          {
            "op": "set",
            "target": "header",
            "data": {
              "name": "Authorization",
              "value": "Basic..."
            }
          }
        ]
      })JSON";
      const auto value = boost::json::parse(content);
      const auto rule = boost::json::value_to<Rule>(value);
      BOOST_TEST(rule.enabled == false);
    }

    BOOST_AUTO_TEST_SUITE_END()

    BOOST_AUTO_TEST_CASE(value_to_http_config_should_return_expected)
    {
      const auto content = R"JSON({
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
      })JSON";
      const auto value = boost::json::parse(content);
      const auto config = boost::json::value_to<Config>(value);
      BOOST_REQUIRE(config.rules.has_value());
      BOOST_TEST(config.rules->size() == 1);
    }
  }
}