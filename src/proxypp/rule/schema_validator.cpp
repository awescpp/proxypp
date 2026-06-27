/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "schema_validator.h"
#include "error.h"
#include <jsoncons/json.hpp>
#include <jsoncons_ext/jsonschema/jsonschema.hpp>

namespace proxypp::rule
{

  namespace
  {
    constexpr std::string_view kJsonSchemaDraft202012
      = "https://json-schema.org/draft/2020-12/schema";
    constexpr std::string_view kJsonSchemaDraft202012WithHash
      = "https://json-schema.org/draft/2020-12/schema#";

    bool IsSupportedJsonSchemaVersion(std::string_view value)
    {
      return value == kJsonSchemaDraft202012
             || value == kJsonSchemaDraft202012WithHash;
    }

    Result<void> CheckJsonSchemaVersion(const jsoncons::json& schema)
    {
      if(!schema.is_object())
        {
          Error error { Errc::InvalidRuleSchema,
                        "rule schema root must be a JSON object" };
          return Unexpected(error);
        }

      const auto it = schema.find("$schema");
      if(it == schema.object_range().end())
        {
          Error error { Errc::UnsupportedJsonSchemaVersion,
                        std::format("rule schema must declare $schema: {}",
                                    kJsonSchemaDraft202012) };
          return Unexpected(error);
        }
      if(!(it->value().is_string()))
        {
          Error error { Errc::UnsupportedJsonSchemaVersion,
                        "rule schema '$schema' must be a string" };
          return Unexpected(error);
        }

      const auto value = it->value().as_string_view();
      if(!IsSupportedJsonSchemaVersion(value))
        {
          Error error { Errc::UnsupportedJsonSchemaVersion,
                        std::format(
                          "unsupported rule schema version: {}, expected: {}",
                          value, kJsonSchemaDraft202012) };
          return Unexpected(error);
        }

      return {};
    }

  }

  Result<void> ValidateJsonBySchema(std::string_view schema_text,
                                    std::string_view document_text)
  {
    namespace jsonschema = jsoncons::jsonschema;
    using jsoncons::json;
    json schema;
    try
      {
        schema = json::parse(std::string { schema_text });
      }
    catch(const jsoncons::ser_error& e)
      {
        Error error { Errc::InvalidRuleSchemaJson,
                      std::format("invalid rule schema JSON: {}", e.what()) };
        return Unexpected(error);
      }

    if(auto result = CheckJsonSchemaVersion(schema); !result.has_value())
      {
        return result;
      }

    json document;

    try
      {
        document = json::parse(std::string { document_text });
      }
    catch(const jsoncons::ser_error& e)
      {
        Error error { Errc::InvalidRuleFileJson,
                      std::format("invalid rule file JSON: {}", e.what()) };
        return Unexpected(error);
      }

    try
      {
        const auto options = jsonschema::evaluation_options {}.default_version(
          std::string { kJsonSchemaDraft202012 });
        auto compiled
          = jsonschema::make_json_schema(std::move(schema), options);
        compiled.validate(document);
      }
    catch(const jsonschema::validation_error& e)
      {
        Error error { Errc::RuleFileSchemaValidationFailed,
                      std::format("rule file schema validation failed: {}",
                                  e.what()) };
        return Unexpected(error);
      }
    catch(const std::exception& e)
      {
        Error error { Errc::InvalidRuleSchema,
                      std::format("invalid rule schema: {}", e.what()) };
        return Unexpected(error);
      }

    return {};
  }
}