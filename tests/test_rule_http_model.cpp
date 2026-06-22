/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#define BOOST_TEST_MODULE rule_http_model_tests

#include "proxypp/rule/http/model.h"
#include <array>
#include <boost/json.hpp>
#include <boost/test/unit_test.hpp>
#include <format>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace json = boost::json;

namespace proxypp::rule::http::test
{
  BOOST_AUTO_TEST_SUITE(PhaseFromStringTests)

  BOOST_AUTO_TEST_CASE(phase_from_string_should_return_request_for_request)
  {
    BOOST_TEST(PhaseFromString("request") == Phase::Request);
  }

  BOOST_AUTO_TEST_CASE(phase_from_string_should_return_response_for_response)
  {
    BOOST_TEST(PhaseFromString("response") == Phase::Response);
  }

  BOOST_AUTO_TEST_CASE(
    phase_from_string_should_throw_invalid_argument_for_invalid_input)
  {
    constexpr std::array<std::string_view, 9> invalid_inputs
      = { "",        " ",        "request ", " response", "response\t",
          "REQUEST", "Response", "Request",  "connect" };
    for(const auto& invalid_input : invalid_inputs)
      {
        BOOST_TEST_CONTEXT("input=" << invalid_input)
        {
          BOOST_CHECK_THROW(PhaseFromString(invalid_input),
                            std::invalid_argument);
        }
      }
  }

  BOOST_AUTO_TEST_SUITE_END()

  BOOST_AUTO_TEST_SUITE(HeaderOpFromStringTests)

  BOOST_AUTO_TEST_CASE(header_op_from_string_should_return_add_for_header_add)
  {
    BOOST_TEST(HeaderOpFromString("header.add") == HeaderOp::Add);
  }

  BOOST_AUTO_TEST_CASE(header_op_from_string_should_return_set_for_header_set)
  {
    BOOST_TEST(HeaderOpFromString("header.set") == HeaderOp::Set);
  }

  BOOST_AUTO_TEST_CASE(
    header_op_from_string_should_return_remove_for_header_remove)
  {
    BOOST_TEST(HeaderOpFromString("header.remove") == HeaderOp::Remove);
  }

  BOOST_AUTO_TEST_CASE(
    header_op_from_string_should_return_replace_for_header_replace)
  {
    BOOST_TEST(HeaderOpFromString("header.replace") == HeaderOp::Replace);
  }

  BOOST_AUTO_TEST_CASE(
    header_op_from_string_should_throw_invalid_argument_for_invalid_input)
  {
    constexpr std::array<std::string_view, 9> invalid_inputs
      = { "",           " ",           "header_add",
          "header.Add", "Header.Add",  "Header.Remove",
          "HEADER.SET", "header.set ", " header.replace" };
    for(const auto& invalid_input : invalid_inputs)
      {
        BOOST_TEST_CONTEXT("input=" << invalid_input)
        {
          BOOST_CHECK_THROW(HeaderOpFromString(invalid_input),
                            std::invalid_argument);
        }
      }
  }

  BOOST_AUTO_TEST_SUITE_END()

  BOOST_AUTO_TEST_SUITE(MatchTagInvokeTests)

  BOOST_AUTO_TEST_CASE(
    value_to_should_convert_valid_json_object_to_match_struct)
  {
    const auto value = json::parse(R"json(
        {
          "expr": "ctx.request.method === 'GET'"
        }
    )json");
    const auto [expr] = json::value_to<Match>(value);
    BOOST_TEST(expr == "ctx.request.method === 'GET'");
  }

  BOOST_AUTO_TEST_CASE(value_to_should_throw_when_expr_is_missing)
  {
    const auto value = json::parse(R"json(
        {}
    )json");
    BOOST_CHECK_THROW(json::value_to<Match>(value), std::exception);
  }

  BOOST_AUTO_TEST_SUITE_END()

  BOOST_AUTO_TEST_SUITE(HeaderSetActionTagInvokeTests)

  BOOST_AUTO_TEST_CASE(
    value_to_should_convert_valid_json_object_to_header_set_action_struct)
  {
    const auto value = json::parse(R"json(
        {
          "op": "header.set",
          "name": "X-PROXYPP-DATA",
          "value": "text"
        }
    )json");
    const auto header_set_action = json::value_to<HeaderSetAction>(value);
    BOOST_TEST(header_set_action.kOp == HeaderOp::Set);
    BOOST_TEST(header_set_action.name == "X-PROXYPP-DATA");
    BOOST_TEST(header_set_action.value == "text");
  }

  BOOST_AUTO_TEST_CASE(
    value_to_should_fail_when_header_action_type_is_not_valid_header_set)
  {
    const std::vector<std::string_view> invalid_header_actions
      = { "header.add", "header.remove", "header.replace",
          "header.Set", "Header.Set",    "",
          " " };

    for(const auto& header_action : invalid_header_actions)
      {
        BOOST_TEST_CONTEXT("header_action=" << header_action)
        {
          const auto value = json::parse(std::format(
            R"json(
            {{
              "op": "{}",
              "name": "X-PROXYPP-DATA",
              "value": "text"
            }})json",
            header_action));
          BOOST_CHECK_THROW(json::value_to<HeaderSetAction>(value),
                            std::exception);
        }
      }
  }

  BOOST_AUTO_TEST_CASE(
    value_to_should_fail_when_json_object_is_missing_required_property)
  {
    std::array<std::string_view, 2> invalid_bodies = {
      R"json(
        {
          "op": "header.set",
          "value": "text"
        }
      )json",
      R"json({})json",
    };
    for(const auto& invalid_json_body : invalid_bodies)
      {
        BOOST_TEST_CONTEXT("body=" << invalid_json_body)
        {
          BOOST_CHECK_THROW(
            json::value_to<HeaderSetAction>(json::parse(invalid_json_body)),
            std::exception);
        }
      }
  }

  BOOST_AUTO_TEST_SUITE_END()

  BOOST_AUTO_TEST_SUITE(HeaderAddActionTagInvokeTests)

  BOOST_AUTO_TEST_CASE(
    value_to_should_convert_valid_json_object_to_header_add_action_struct)
  {
    const auto value = json::parse(R"json(
        {
          "op": "header.add",
          "name": "X-PROXYPP-DATA",
          "value": "text"
        }
    )json");
    const auto header_add_action = json::value_to<HeaderAddAction>(value);
    BOOST_TEST(header_add_action.kOp == HeaderOp::Add);
    BOOST_TEST(header_add_action.name == "X-PROXYPP-DATA");
    BOOST_TEST(header_add_action.value == "text");
  }

  BOOST_AUTO_TEST_CASE(
    value_to_should_fail_when_header_action_type_is_not_valid_header_add)
  {
    const std::vector<std::string_view> invalid_header_actions
      = { "header.set", "header.remove", "header.replace",
          "header.Add", "Header.Add",    "",
          " " };

    for(const auto& header_action : invalid_header_actions)
      {
        BOOST_TEST_CONTEXT("header_action=" << header_action)
        {
          const auto value = json::parse(std::format(
            R"json(
            {{
              "op": "{}",
              "name": "X-PROXYPP-DATA",
              "value": "text"
            }})json",
            header_action));
          BOOST_CHECK_THROW(json::value_to<HeaderAddAction>(value),
                            std::exception);
        }
      }
  }

  BOOST_AUTO_TEST_CASE(
    value_to_should_fail_when_json_object_is_missing_required_property)
  {
    std::array<std::string_view, 2> invalid_bodies = {
      R"json(
        {
          "op": "header.add",
          "name": "property_to_remove"
        }
      )json",
      R"json({})json",
    };
    for(const auto& invalid_json_body : invalid_bodies)
      {
        BOOST_TEST_CONTEXT("body=" << invalid_json_body)
        {
          BOOST_CHECK_THROW(
            json::value_to<HeaderAddAction>(json::parse(invalid_json_body)),
            std::exception);
        }
      }
  }

  BOOST_AUTO_TEST_SUITE_END()

  BOOST_AUTO_TEST_SUITE(HeaderReplaceActionTagInvokeTests)

  BOOST_AUTO_TEST_CASE(
    value_to_should_convert_valid_json_object_to_header_replace_action_struct)
  {
    const auto value = json::parse(R"json(
        {
          "op": "header.replace",
          "name": "X-PROXYPP-DATA",
          "value": "text"
        }
    )json");
    const auto header_replace_action
      = json::value_to<HeaderReplaceAction>(value);
    BOOST_TEST(header_replace_action.kOp == HeaderOp::Replace);
    BOOST_TEST(header_replace_action.name == "X-PROXYPP-DATA");
    BOOST_TEST(header_replace_action.value == "text");
  }

  BOOST_AUTO_TEST_CASE(
    value_to_should_fail_when_header_action_type_is_not_valid_header_replace)
  {
    const std::vector<std::string_view> invalid_header_actions
      = { "header.set",
          "header.remove",
          "header.add",
          "header.Replace",
          "Header.Replace",
          "",
          " " };

    for(const auto& header_action : invalid_header_actions)
      {
        BOOST_TEST_CONTEXT("header_action=" << header_action)
        {
          const auto value = json::parse(std::format(
            R"json(
            {{
              "op": "{}",
              "name": "X-PROXYPP-DATA",
              "value": "text"
            }})json",
            header_action));
          BOOST_CHECK_THROW(json::value_to<HeaderReplaceAction>(value),
                            std::exception);
        }
      }
  }

  BOOST_AUTO_TEST_CASE(
    value_to_should_fail_when_json_object_is_missing_required_property)
  {
    std::array<std::string_view, 2> invalid_bodies = {
      R"json(
        {
          "op": "header.replace",
          "name": "property_to_remove"
        }
      )json",
      R"json({})json",
    };
    for(const auto& invalid_json_body : invalid_bodies)
      {
        BOOST_TEST_CONTEXT("body=" << invalid_json_body)
        {
          BOOST_CHECK_THROW(json::value_to<HeaderReplaceAction>(
                              json::parse(invalid_json_body)),
                            std::exception);
        }
      }
  }

  BOOST_AUTO_TEST_SUITE_END()

  BOOST_AUTO_TEST_SUITE(HeaderRemoveActionTagInvokeTests)

  BOOST_AUTO_TEST_CASE(
    value_to_should_convert_valid_json_object_to_header_remove_action_struct)
  {
    const auto value = json::parse(R"json(
        {
          "op": "header.remove",
          "name": "X-PROXYPP-DATA"
        }
    )json");
    const auto header_remove_action
      = json::value_to<HeaderRemoveAction>(value);
    BOOST_TEST(header_remove_action.kOp == HeaderOp::Remove);
    BOOST_TEST(header_remove_action.name == "X-PROXYPP-DATA");
  }

  BOOST_AUTO_TEST_CASE(
    value_to_should_fail_when_json_object_is_missing_required_property)
  {
    std::array<std::string_view, 2> invalid_bodies = {
      R"json(
        {
          "op": "header.remove"
        }
      )json",
      R"json(
        {
          "name": "property_to_remove"
        }
      )json",
    };
    for(const auto& invalid_json_body : invalid_bodies)
      {
        BOOST_TEST_CONTEXT("body=" << invalid_json_body)
        {
          BOOST_CHECK_THROW(
            json::value_to<HeaderRemoveAction>(json::parse(invalid_json_body)),
            std::exception);
        }
      }
  }

  BOOST_AUTO_TEST_CASE(
    value_to_should_fail_when_header_action_type_is_not_valid_header_remove)
  {
    const std::vector<std::string_view> invalid_header_actions
      = { "header.set",
          "header.replace",
          "header.add",
          "header.Remove",
          "Header.Remove",
          "",
          " " };

    for(const auto& header_action : invalid_header_actions)
      {
        BOOST_TEST_CONTEXT("header_action=" << header_action)
        {
          const auto value = json::parse(std::format(
            R"json(
            {{
              "op": "{}",
              "name": "X-PROXYPP-DATA"
            }})json",
            header_action));
          BOOST_CHECK_THROW(json::value_to<HeaderRemoveAction>(value),
                            std::exception);
        }
      }
  }

  BOOST_AUTO_TEST_SUITE_END()

  BOOST_AUTO_TEST_SUITE(HeaderActionTagInvokeTests)

  BOOST_AUTO_TEST_CASE(
    value_to_should_convert_valid_json_object_to_header_add_action_struct)
  {
    const auto value = json::parse(R"json(
        {
          "op": "header.add",
          "name": "X-PROXYPP-DATA",
          "value": "text"
        }
    )json");
    const auto header_action = json::value_to<Action>(value);
    BOOST_REQUIRE(std::holds_alternative<HeaderAddAction>(header_action));
    const auto& header_add_action = std::get<HeaderAddAction>(header_action);
    BOOST_TEST(header_add_action.name == "X-PROXYPP-DATA");
    BOOST_TEST(header_add_action.value == "text");
  }

  BOOST_AUTO_TEST_CASE(
    value_to_should_convert_valid_json_object_to_header_set_action_struct)
  {
    const auto value = json::parse(R"json(
        {
          "op": "header.set",
          "name": "X-PROXYPP-DATA",
          "value": "text"
        }
    )json");
    const auto header_action = json::value_to<Action>(value);
    BOOST_REQUIRE(std::holds_alternative<HeaderSetAction>(header_action));
    const auto& header_set_action = std::get<HeaderSetAction>(header_action);
    BOOST_TEST(header_set_action.name == "X-PROXYPP-DATA");
    BOOST_TEST(header_set_action.value == "text");
  }

  BOOST_AUTO_TEST_CASE(
    value_to_should_convert_valid_json_object_to_header_replace_action_struct)
  {
    const auto value = json::parse(R"json(
        {
          "op": "header.replace",
          "name": "X-PROXYPP-DATA",
          "value": "text"
        }
    )json");
    const auto header_action = json::value_to<Action>(value);
    BOOST_REQUIRE(std::holds_alternative<HeaderReplaceAction>(header_action));
    const auto& header_replace_action
      = std::get<HeaderReplaceAction>(header_action);
    BOOST_TEST(header_replace_action.name == "X-PROXYPP-DATA");
    BOOST_TEST(header_replace_action.value == "text");
  }

  BOOST_AUTO_TEST_CASE(
    value_to_should_convert_valid_json_object_to_header_remove_action_struct)
  {
    const auto value = json::parse(R"json(
        {
          "op": "header.remove",
          "name": "X-PROXYPP-DATA"
        }
    )json");
    const auto header_action = json::value_to<Action>(value);
    BOOST_REQUIRE(std::holds_alternative<HeaderRemoveAction>(header_action));
    const auto& header_remove_action
      = std::get<HeaderRemoveAction>(header_action);
    BOOST_TEST(header_remove_action.name == "X-PROXYPP-DATA");
  }

  BOOST_AUTO_TEST_CASE(
    value_to_should_fail_when_json_object_is_missing_required_property)
  {
    const auto value = json::parse(R"json(
        {
          "op": "header.add",
          "name": "X-PROXYPP-DATA"
        }
    )json");
    BOOST_CHECK_THROW(json::value_to<Action>(value), std::exception);
  }

  BOOST_AUTO_TEST_SUITE_END()

  BOOST_AUTO_TEST_SUITE(RuleTagInvokeTests)

  BOOST_AUTO_TEST_CASE(value_to_should_convert_valid_json_object_to_rule_struct)
  {
    const auto value = json::parse(R"json(
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
    )json");
    const auto rule = json::value_to<Rule>(value);
    BOOST_TEST(rule.name
               == "add basic authorization header for 192.168.52.101:3067");
    BOOST_TEST(rule.phase == Phase::Request);
    BOOST_TEST(rule.actions.size() == 1);
    BOOST_REQUIRE(std::holds_alternative<HeaderSetAction>(rule.actions[0]));
    const auto action = std::get<HeaderSetAction>(rule.actions[0]);
    BOOST_TEST(action.name == "Authorization");
    BOOST_TEST(action.value == "Basic dXNlcjpwYXNzd29yZA==");

    BOOST_REQUIRE(rule.match.has_value());
    BOOST_TEST(
      rule.match->expr
      == "ctx.request.host==='192.168.52.101' && ctx.request.port===3067");
  }

  BOOST_AUTO_TEST_CASE(
    value_to_should_convert_json_object_with_zero_actions_to_rule_struct)
  {
    const auto value = json::parse(R"json(
       {
        "name": "add basic authorization header for 192.168.52.101:3067",
        "phase": "request",
        "actions": []
      }
    )json");
    const auto rule = json::value_to<Rule>(value);
    BOOST_TEST(rule.name
               == "add basic authorization header for 192.168.52.101:3067");
    BOOST_TEST(rule.phase == Phase::Request);
    BOOST_TEST(rule.actions.size() == 0);
    BOOST_TEST(!rule.match.has_value());
  }

  BOOST_AUTO_TEST_CASE(
    value_to_should_fail_when_json_object_missing_action_which_is_a_required_property)
  {
    const auto value = json::parse(R"json(
       {
        "name": "add basic authorization header for 192.168.52.101:3067",
        "phase": "request"
      }
    )json");
    BOOST_CHECK_THROW(json::value_to<Rule>(value), std::exception);
  }

  BOOST_AUTO_TEST_CASE(
    value_to_should_default_enabled_to_true_when_enabled_is_missing)
  {
    const auto value = json::parse(R"json(
    {
      "name": "rule without enabled",
      "phase": "request",
      "actions": []
    }
  )json");
    const auto rule = json::value_to<Rule>(value);
    BOOST_TEST(rule.enabled);
  }

  BOOST_AUTO_TEST_CASE(value_to_should_parse_enabled_false)
  {
    const auto value = json::parse(R"json(
    {
      "name": "disabled rule",
      "enabled": false,
      "phase": "request",
      "actions": []
    }
  )json");

    const auto rule = json::value_to<Rule>(value);
    BOOST_TEST(!rule.enabled);
  }

  BOOST_AUTO_TEST_CASE(value_to_should_fail_when_rule_phase_is_invalid)
  {
    const auto value = json::parse(R"json(
       {
        "name": "add basic authorization header for 192.168.52.101:3067",
        "phase": "request ",
        "match": {
          "expr": "ctx.request.host==='192.168.52.101' && ctx.request.port===3067"
        },
        "actions": []
      }
    )json");
    BOOST_CHECK_THROW(json::value_to<Rule>(value), std::exception);
  }

  BOOST_AUTO_TEST_CASE(value_to_should_fail_when_rule_name_is_missing)
  {
    const auto value = json::parse(R"json(
       {
        "phase": "request",
        "match": {
          "expr": "ctx.request.host==='192.168.52.101' && ctx.request.port===3067"
        },
        "actions": []
      }
    )json");
    BOOST_CHECK_THROW(json::value_to<Rule>(value), std::exception);
  }

  BOOST_AUTO_TEST_CASE(value_to_should_fail_when_match_expr_is_missing)
  {
    const auto value = json::parse(R"json(
       {
        "name": "add basic authorization header for 192.168.52.101:3067",
        "phase": "request",
        "match": {},
        "actions": []
      }
    )json");
    BOOST_CHECK_THROW(json::value_to<Rule>(value), std::exception);
  }

  BOOST_AUTO_TEST_SUITE_END()

  BOOST_AUTO_TEST_SUITE(HTTPConfigTagInvokeTests)

  BOOST_AUTO_TEST_CASE(
    value_to_should_convert_valid_json_object_to_http_config_struct)
  {
    const auto value = json::parse(R"json(
       {
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
    )json");
    const auto http_config = json::value_to<http::Config>(value);
    BOOST_REQUIRE(http_config.rules.has_value());
    BOOST_TEST(http_config.rules->size() == 1);
  }

  BOOST_AUTO_TEST_CASE(
    value_to_should_convert_valid_json_object_with_zero_rules_to_http_config_struct)
  {
    const auto value = json::parse(R"json(
       {
        "rules": []
        }
    )json");
    const auto http_config = json::value_to<http::Config>(value);
    BOOST_REQUIRE(http_config.rules.has_value());
    BOOST_TEST(http_config.rules->size() == 0);
  }

  BOOST_AUTO_TEST_CASE(
    value_to_should_convert_valid_json_object_without_rules_property_to_http_config_struct)
  {
    const auto value = json::parse(R"json({})json");
    const auto http_config = json::value_to<http::Config>(value);
    BOOST_TEST(!http_config.rules.has_value());
  }

  BOOST_AUTO_TEST_CASE(value_to_should_fail_when_http_rules_is_not_array)
  {
    const auto value = json::parse(R"json(
       {
        "rules": {}
       }
    )json");
    BOOST_CHECK_THROW(json::value_to<http::Config>(value), std::exception);
  }

  BOOST_AUTO_TEST_CASE(
    value_to_should_fail_when_http_rules_contains_invalid_rule)
  {
    const auto value = json::parse(R"json(
       {
        "rules": [
          {
            "name": "add basic authorization header for 192.168.52.101:3067",
            "phase": "request",
            "match": {},
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
    )json");
    BOOST_CHECK_THROW(json::value_to<http::Config>(value), std::exception);
  }

  BOOST_AUTO_TEST_SUITE_END()

}