/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#define BOOST_TEST_MODULE schema_helper_tests

#include "proxypp/error.h"
#include "proxypp/helper/json_schema_helper.h"
#include <boost/test/unit_test.hpp>

namespace proxypp::helper::schema
{
  namespace test
  {
    BOOST_AUTO_TEST_CASE(validate_valid_document_should_return_success)
    {
      const auto schema_text = R"JSON({
        "$schema": "https://json-schema.org/draft/2020-12/schema",
        "type": "object",
        "required": ["message", "code"],
        "properties": {
          "message": {"type": "string"},
          "code": {"type": "integer"}
        }
      })JSON";
      const auto document_text = R"JSON({
          "message": "test",
          "code": 42
      })JSON";
      const auto result = ValidateJsonBySchema(schema_text, document_text);
      if(!result.has_value())
        {
          BOOST_TEST_FAIL(result.error().message);
        }
      BOOST_REQUIRE(result.has_value());
    }

    BOOST_AUTO_TEST_CASE(validate_should_fail_with_invalid_schema_json)
    {
      // remove closing quote from "object" to make a syntax error
      const auto schema_text = R"JSON({
        "$schema": "https://json-schema.org/draft/2020-12/schema",
        "type": "object,
        "required": ["message", "code"],
        "properties": {
          "message": {"type": "string"},
          "code": {"type": "integer"}
        }
      })JSON";
      const auto document_text = R"JSON({
          "message": "test",
          "code": 42
      })JSON";
      const auto result = ValidateJsonBySchema(schema_text, document_text);
      BOOST_REQUIRE(result.has_value() == false);
      BOOST_TEST(result.error().code == Errc::InvalidJsonSchema);
    }

    BOOST_AUTO_TEST_CASE(validate_should_fail_with_invalid_document_json)
    {
      const auto schema_text = R"JSON({
        "$schema": "https://json-schema.org/draft/2020-12/schema",
        "type": "object",
        "required": ["message", "code"],
        "properties": {
          "message": {"type": "string"},
          "code": {"type": "integer"}
        }
      })JSON";
      // add a comma after 'code' to make a syntax error
      const auto document_text = R"JSON({
          "message": "test",
          "code": 42,
      })JSON";
      const auto result = ValidateJsonBySchema(schema_text, document_text);
      BOOST_REQUIRE(result.has_value() == false);
      BOOST_TEST(result.error().code == Errc::JsonParseFailed);
    }

    BOOST_AUTO_TEST_CASE(
      validate_should_return_validation_error_when_document_not_matching_schema)
    {
      const auto schema_text = R"JSON({
        "$schema": "https://json-schema.org/draft/2020-12/schema",
        "type": "object",
        "required": ["message", "code"],
        "properties": {
          "message": {"type": "string"},
          "code": {"type": "integer"}
        }
      })JSON";
      const auto document_text = R"JSON({
          "message": "test",
          "code": "42"
      })JSON";
      const auto result = ValidateJsonBySchema(schema_text, document_text);
      BOOST_REQUIRE(result.has_value() == false);
      BOOST_TEST(result.error().code == Errc::JsonSchemaValidationError);
    }

    BOOST_AUTO_TEST_CASE(
      validate_invalid_schema_should_return_invalid_rule_schema)
    {
      // made 'required' a object to make a semantic error to parse schema
      const auto schema_text = R"JSON({
        "$schema": "https://json-schema.org/draft/2020-12/schema",
        "type": "object",
        "required": { "message": "code" },
        "properties": {
          "message": {"type": "string"},
          "code": {"type": "integer"}
        }
      })JSON";
      const auto document_text = R"JSON({
          "message": "test",
          "code": 42
      })JSON";
      const auto result = ValidateJsonBySchema(schema_text, document_text);
      BOOST_REQUIRE(result.has_value() == false);
      BOOST_TEST(result.error().code == Errc::InvalidJsonSchema);
    }

    BOOST_AUTO_TEST_CASE(
      validate_schema_without_schema_version_should_return_unsupported_rule_schema_version)
    {
      const auto schema_text = R"JSON({
        "type": "object",
        "required": ["message", "code"],
        "properties": {
          "message": {"type": "string"},
          "code": {"type": "integer"}
        }
      })JSON";
      const auto document_text = R"JSON({
          "message": "test",
          "code": 42
      })JSON";
      const auto result = ValidateJsonBySchema(schema_text, document_text);
      BOOST_REQUIRE(result.has_value() == false);
      BOOST_TEST(result.error().code == Errc::UnsupportedJsonSchemaVersion);
    }

    BOOST_AUTO_TEST_CASE(
      validate_schema_with_non_string_schema_version_should_return_unsupported_rule_schema_version)
    {
      const auto schema_text = R"JSON({
        "$schema": 202012,
        "type": "object",
        "required": ["message", "code"],
        "properties": {
          "message": {"type": "string"},
          "code": {"type": "integer"}
        }
      })JSON";
      const auto document_text = R"JSON({
          "message": "test",
          "code": 42
      })JSON";
      const auto result = ValidateJsonBySchema(schema_text, document_text);
      BOOST_REQUIRE(result.has_value() == false);
      BOOST_TEST(result.error().code == Errc::UnsupportedJsonSchemaVersion);
    }

    BOOST_AUTO_TEST_CASE(
      validate_schema_with_draft_07_should_return_unsupported_rule_schema_version)
    {
      const auto schema_text = R"JSON({
        "$schema": "https://json-schema.org/draft-07/schema#",
        "type": "object",
        "required": ["message", "code"],
        "properties": {
          "message": {"type": "string"},
          "code": {"type": "integer"}
        }
      })JSON";
      const auto document_text = R"JSON({
          "message": "test",
          "code": 42
      })JSON";
      const auto result = ValidateJsonBySchema(schema_text, document_text);
      BOOST_REQUIRE(result.has_value() == false);
      BOOST_TEST_MESSAGE(result.error().message);
      BOOST_TEST(result.error().code == Errc::UnsupportedJsonSchemaVersion);
    }

    // validate_schema_with_draft_2020_12_should_return_success reference
    // validate_valid_document_should_return_expected

  }
}