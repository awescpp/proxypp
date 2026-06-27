/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#define BOOST_TEST_MODULE test_rule_schema_validation

#include <boost/test/unit_test.hpp>
#include <filesystem>
#include <fstream>
#include <proxypp/rule/error.h>
#include <proxypp/rule/schema_validator.h>
#include <sstream>

namespace proxypp::rule
{
  namespace
  {
    std::string ReadTextFile(const std::filesystem::path& path)
    {
      std::ifstream file { path, std::ios::binary };
      BOOST_REQUIRE_MESSAGE(file.is_open(),
                            "failed to open file: " << path.string());
      std::ostringstream oss;
      oss << file.rdbuf();
      return oss.str();
    }
  }

  namespace test
  {
    struct SchemaValidationFixture
    {
      std::string schema_text;
      SchemaValidationFixture()
          : schema_text(ReadTextFile(PROXYPP_RULE_SCHEMA_V1_PATH))
      {}
    };

    BOOST_FIXTURE_TEST_SUITE(schema_validation_tests, SchemaValidationFixture)

    BOOST_AUTO_TEST_CASE(minimal_config_should_pass_schema_validation)
    {
      const auto document = R"JSON({
        "version": 1
      })JSON";
      const auto result = ValidateJsonBySchema(schema_text, document);
      BOOST_TEST(result.has_value());
    }

    BOOST_AUTO_TEST_CASE(config_with_http_rule_should_pass_schema_validation)
    {
      const auto document = R"JSON({
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
      })JSON";
      const auto result = ValidateJsonBySchema(schema_text, document);
      if(!result.has_value())
        {
          BOOST_TEST_FAIL(result.error().message);
        }
      BOOST_REQUIRE(result.has_value());
    }

    BOOST_AUTO_TEST_CASE(header_remove_with_name_value_data_should_fail)
    {
      const auto document = R"JSON({
        "$schema": "../../schemas/proxypp_rules_schema_v1.json",
        "version": 1,
        "http": {
          "rules": [
            {
              "name": "remove user-agent for all outgoing requests",
              "phase": "request",
              "actions": [
                {
                  "op": "remove",
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
      })JSON";
      const auto result = ValidateJsonBySchema(schema_text, document);
      BOOST_REQUIRE(result.has_value() == false);
      BOOST_TEST(result.error().code == Errc::RuleFileSchemaValidationFailed);
    }

    BOOST_AUTO_TEST_CASE(unknown_top_level_property_should_fail)
    {
      const auto document = R"JSON({
        "version": 1,
        "author": "awescpp"
      })JSON";
      const auto result = ValidateJsonBySchema(schema_text, document);
      BOOST_REQUIRE(result.has_value() == false);
      BOOST_TEST(result.error().code == Errc::RuleFileSchemaValidationFailed);
    }

    BOOST_AUTO_TEST_SUITE_END()

  }
}