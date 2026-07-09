/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "proxypp/json.h"
#include "proxypp/rule/match.h"
#include "proxypp/rule/op.h"

namespace proxypp::rule::http
{
  namespace data
  {
    struct HeaderNameData
    {
      std::string name;
    };

    struct HeaderNameValueData
    {
      std::string name;
      std::string value;
    };
  }

  enum class Phase
  {
    NotSet = 0,
    Request,
    Response
  };

  Phase PhaseFromString(std::string_view value);

  std::ostream& operator<<(std::ostream& os, Phase value);

  enum class Target
  {
    NotSet = 0,
    Header
  };

  Target TargetFromString(std::string_view value);

  std::ostream& operator<<(std::ostream& os, Target value);

  using ActionData
    = std::variant<data::HeaderNameData, data::HeaderNameValueData>;

  struct Action
  {
    rule::Op op = rule::Op::NotSet;
    Target target = Target::NotSet;
    ActionData data;
  };

  struct Rule
  {
    std::string name;
    bool enabled = true;
    Phase phase = Phase::NotSet;
    std::optional<rule::Match> match;
    std::vector<Action> actions;
  };

  struct Config
  {
    std::optional<std::vector<Rule>> rules;
  };

}

JSONCONS_ALL_MEMBER_TRAITS(proxypp::rule::http::data::HeaderNameData, name)

JSONCONS_ALL_MEMBER_TRAITS(proxypp::rule::http::data::HeaderNameValueData,
                           name, value)

JSONCONS_ENUM_NAME_TRAITS(proxypp::rule::http::Phase, (Request, "request"),
                          (Response, "response"))

JSONCONS_ENUM_NAME_TRAITS(proxypp::rule::http::Target, (Header, "header"))

namespace jsoncons
{
  template <typename Json>
  struct json_type_traits<Json, proxypp::rule::http::Action>
  {
    static bool is(const Json& j) noexcept
    {
      return j.is_object() && j.contains("op") && j.contains("target")
             && j.contains("data");
    }

    static proxypp::rule::http::Action as(const Json& j)
    {
      using proxypp::rule::Op;
      using proxypp::rule::http::Action;
      using proxypp::rule::http::Target;
      namespace data = proxypp::rule::http::data;

      Action action;
      action.op = j.at("op").template as<Op>();
      action.target = j.at("target").template as<Target>();

      const auto& data_json = j.at("data");
      if(action.target == Target::Header)
        {
          if(action.op == Op::Add || action.op == Op::Set
             || action.op == Op::Replace)
            {
              action.data = data_json.template as<data::HeaderNameValueData>();
            }
          else if(action.op == Op::Remove)
            {
              action.data = data_json.template as<data::HeaderNameData>();
            }
          else
            {
              throw jsoncons::conv_error(
                jsoncons::conv_errc::conversion_failed, "invalid action op");
            }
        }
      else
        {
          throw jsoncons::conv_error(jsoncons::conv_errc::conversion_failed,
                                     "unsupported action target");
        }
      return action;
    }
  };
}

JSONCONS_N_MEMBER_TRAITS(proxypp::rule::http::Rule, 3, name, phase, actions,
                         enabled, match)

JSONCONS_N_MEMBER_TRAITS(proxypp::rule::http::Config, 0, rules)