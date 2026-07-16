/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "json_schema_helper.h"
#include "proxypp/error.h"
#include <jsoncons_ext/jsonschema/jsonschema.hpp>
#include <proxypp/json.h>

namespace proxypp::helper::schema
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
          return Unexpected(Error { Errc::InvalidJsonSchema,
                                    "rule schema must be a JSON object" });
        }

      const auto it = schema.find("$schema");
      if(it == schema.object_range().end())
        {
          return Unexpected(
            Error { Errc::InvalidJsonSchema,
                    std::format("rule schema must declare $schema: {}",
                                kJsonSchemaDraft202012) });
        }
      if(!(it->value().is_string()))
        {
          return Unexpected(
            Error { Errc::InvalidJsonSchema,
                    "rule schema '$schema' must be a string" });
        }

      const auto value = it->value().as_string_view();
      if(!IsSupportedJsonSchemaVersion(value))
        {
          return Unexpected(Error {
            Errc::InvalidJsonSchema,
            std::format("unsupported rule schema version: {}, expected: {}",
                        value, kJsonSchemaDraft202012) });
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
        return Unexpected(
          Error { Errc::JsonParseFailed,
                  std::format("parse rule schema failed, {}", e.what()) });
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
        return Unexpected(
          Error { Errc::JsonParseFailed,
                  std::format("invalid rule file JSON: {}", e.what()) });
      }

    try
      {
        const auto options = jsonschema::evaluation_options {}.default_version(
          std::string { kJsonSchemaDraft202012 });
        auto compiled
          = jsonschema::make_json_schema(std::move(schema), options);
        compiled.validate(document);
      }
    catch(const jsonschema::schema_error& e)
      {
        return Unexpected(
          Error { Errc::InvalidJsonSchema,
                  std::format("invalid JSON schema: {}", e.what()) });
      }
    catch(const jsoncons::conv_error& e)
      {
        // schema 序列化失败时会报这个异常
        return Unexpected(
          Error { Errc::InvalidJsonSchema,
                  std::format("invalid JSON schema: {}", e.what()) });
      }
    catch(const jsonschema::validation_error& e)
      {
        return Unexpected(
          Error { Errc::JsonSchemaValidationFailed,
                  std::format("schema validation failed: {}", e.what()) });
      }
    catch(const std::exception& e)
      {
        return Unexpected(
          Error { Errc::InternalError,
                  std::format("schema validation failed, {}", e.what()) });
      }

    return {};
  }
}