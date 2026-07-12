/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "proxypp/rule/rule_file.h"
#include "proxypp/helper/file_helper.h"
#include "proxypp/helper/json_helper.h"
#include "proxypp/helper/json_schema_helper.h"
#include <string_view>

namespace proxypp::rule
{

  namespace
  {
    constexpr std::string_view kRuleSchema = R"JSON(
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://proxypp.net/schemas/proxypp_rules_schema_v1.json",
  "title": "proxy++ rules schema",
  "description": "Defines the JSON configuration format for proxy++ rule-based HTTP/HTTPS traffic processing. This schema describes the structure, fields, allowed values, and static validation rules for proxy++ rules, including processing phases, match conditions, and rule actions. Runtime execution semantics and advanced semantic validation are implemented by proxy++ itself.",
  "type": "object",
  "additionalProperties": false,
  "required": ["version"],
  "properties": {
    "$schema": {
      "type": "string",
      "description": "Optional schema URI used by editors and JSON Schema validators. proxy++ ignores this field when loading rules."
    },
    "version": {
      "type": "integer",
      "const": 1,
      "description": "Configuration schema version"
    },
    "http": {
      "$ref": "#/$defs/http/$defs/config"
    }
  },
  "$defs": {
    "match": {
      "type": "object",
      "additionalProperties": false,
      "required": ["expr"],
      "properties": {
        "expr": {
          "type": "string",
          "minLength": 1,
          "description": "A JavaScript expression evaluated by QuickJS. The expression must return a boolean value. The current HTTP processing context is exposed through the ctx object."
        }
      }
    },
    "op": {
      "type": "string",
      "enum": ["set", "add", "replace", "remove"],
      "description": "Generic action operation. The final semantics are defined by the combination of op, target and data."
    },
    "http": {
      "$defs": {
        "phase": {
          "type": "string",
          "enum": ["request", "response"],
          "description": "The HTTP proxy processing phase in which this rule is evaluated. The request phase is evaluated after an HTTP request has been parsed and before it is forwarded upstream. The response phase is evaluated after an HTTP response has been parsed and before it is forwarded to the client."
        },
        "target": {
          "type": "string",
          "enum": ["header"],
          "description": "HTTP action target. More HTTP targets, such as body, method, URI, or status code, may be added in future schema versions"
        },
        "header": {
          "$defs": {
            "name": {
              "type": "string",
              "minLength": 1,
              "pattern": "^[!#$%&'*+.^_`|~0-9A-Za-z-]+$",
              "description": "HTTP header field name."
            },
            "value": {
              "type": "string",
              "description": "HTTP header field value"
            }
          }
        },
        "data": {
          "$defs": {
            "headerName": {
              "type": "object",
              "additionalProperties": false,
              "required": ["name"],
              "properties": {
                "name": {
                  "$ref": "#/$defs/http/$defs/header/$defs/name"
                }
              }
            },
            "headerNameValue": {
              "type": "object",
              "additionalProperties": false,
              "required": ["name", "value"],
              "properties": {
                "name": {
                  "$ref": "#/$defs/http/$defs/header/$defs/name"
                },
                "value": {
                  "$ref": "#/$defs/http/$defs/header/$defs/value"
                }
              }
            }
          }
        },
        "action": {
          "type": "object",
          "additionalProperties": false,
          "required": ["op", "target", "data"],
          "properties": {
            "op": { "$ref": "#/$defs/op" },
            "target": { "$ref": "#/$defs/http/$defs/target" },
            "data": {
              "description": "Action data. The required shape is determined by the combination of op and target"
            }
          },
          "allOf": [
            {
              "if": {
                "required": ["target", "op"],
                "properties": {
                  "target": { "const": "header" },
                  "op": { "enum": ["set", "add", "replace"] }
                }
              },
              "then": {
                "properties": {
                  "data": {
                    "$ref": "#/$defs/http/$defs/data/$defs/headerNameValue"
                  }
                }
              }
            },
            {
              "if": {
                "required": ["target", "op"],
                "properties": {
                  "target": { "const": "header" },
                  "op": { "const": "remove" }
                }
              },
              "then": {
                "properties": {
                  "data": {
                    "$ref": "#/$defs/http/$defs/data/$defs/headerName"
                  }
                }
              }
            }
          ]
        },
        "rule": {
          "type": "object",
          "additionalProperties": false,
          "required": ["name", "phase", "actions"],
          "properties": {
            "name": {
              "type": "string",
              "minLength": 1,
              "description": "Human-readable rule name."
            },
            "enabled": {
              "type": "boolean",
              "default": true,
              "description": "Whether this rule is enabled"
            },
            "phase": { "$ref": "#/$defs/http/$defs/phase" },
            "match": {
              "$ref": "#/$defs/match",
              "description": "Optional match condition. If omitted, the rule matches all HTTP contexts in the specified phase."
            },
            "actions": {
              "type": "array",
              "minItems": 1,
              "description": "Actions to execute when this rule matches. Actions are executed in declaration order.",
              "items": { "$ref": "#/$defs/http/$defs/action" }
            }
          }
        },
        "config": {
          "type": "object",
          "additionalProperties": false,
          "required": [],
          "properties": {
            "rules": {
              "type": "array",
              "description": "HTTP/HTTPS rule list. Rules are evaluated in declaration order. Each enabled rule is evaluated independently. If a rule matches, its actions are executed in declaration order, and evaluation continues with the next rule.",
              "items": {
                "$ref": "#/$defs/http/$defs/rule"
              }
            }
          }
        }
      }
    }
  }
}
    )JSON";
  }

  Result<rule::Config>
  LoadRulesFromFile(const std::filesystem::path& file_path)
  {
    auto read_file = helper::file::ReadTextFile(file_path);
    if(!read_file)
      {
        return Unexpected(read_file.error());
      }

    auto validation
      = helper::schema::ValidateJsonBySchema(kRuleSchema, *read_file);
    if(!validation)
      {
        return Unexpected(validation.error());
      }

    auto parse_json = helper::json::ParseJson(*read_file);
    if(!parse_json)
      {
        return Unexpected(parse_json.error());
      }

    auto conversion = helper::json::Convert<rule::Config>(*parse_json);
    if(!conversion)
      {
        return Unexpected(conversion.error());
      }
    return *conversion;
  }

}