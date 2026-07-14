/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#define BOOST_TEST_MODULE test_rule_load_file

#include "proxypp/rule/rule_file.h"
#include "test_helper.h"
#include <boost/test/unit_test.hpp>
#include <filesystem>

namespace proxypp::rule::test
{
  constexpr std::string_view kValidRuleFile = R"JSON(
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
}
  )JSON";

  BOOST_AUTO_TEST_CASE(
    load_rules_from_file_should_return_config_when_file_content_is_valid)
  {
    const auto rule_file_path
      = proxypp::test::helper::CreateTempFile(kValidRuleFile, ".json");
    BOOST_REQUIRE(rule_file_path.has_value());

    const auto load_result = LoadRulesFromFile(*rule_file_path);
    BOOST_TEST(load_result.has_value());

    if(rule_file_path.has_value())
      {
        using namespace std::filesystem;
        const bool removed = remove(*rule_file_path);
        BOOST_REQUIRE(removed);
      }
  }

  BOOST_AUTO_TEST_CASE(
    load_rules_from_file_should_return_error_when_file_does_not_exist)
  {
    using namespace std::filesystem;
    path rule_file_path = temp_directory_path() / "unexisted.json";
    BOOST_REQUIRE(!exists(rule_file_path));

    const auto load_result = LoadRulesFromFile(rule_file_path);
    BOOST_REQUIRE(!load_result.has_value());
    BOOST_TEST(load_result.error().code() == Errc::InvalidArgument);
  }

}