/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "proxypp/rule/http/model.h"
#include <format>
#include <magic_enum/magic_enum.hpp>
#include <stdexcept>

std::ostream& proxypp::rule::http::operator<<(std::ostream& os, Phase value)
{
  return os << magic_enum::enum_name(value);
}

std::ostream& proxypp::rule::http::operator<<(std::ostream& os, HeaderOp value)
{
  return os << magic_enum::enum_name(value);
}

proxypp::rule::http::Phase
proxypp::rule::http::PhaseFromString(std::string_view value)
{
  if(value == "request")
    {
      return Phase::Request;
    }
  if(value == "response")
    {
      return Phase::Response;
    }
  throw std::invalid_argument("invalid HTTP rule phase: "
                              + std::string(value));
}

proxypp::rule::http::HeaderOp
proxypp::rule::http::HeaderOpFromString(std::string_view value)
{
  if(value == "header.add")
    {
      return HeaderOp::Add;
    }
  if(value == "header.set")
    {
      return HeaderOp::Set;
    }
  if(value == "header.remove")
    {
      return HeaderOp::Remove;
    }
  if(value == "header.replace")
    {
      return HeaderOp::Replace;
    }
  throw std::invalid_argument("invalid HTTP header action op: "
                              + std::string(value));
}

proxypp::rule::http::Match
proxypp::rule::http::tag_invoke(boost::json::value_to_tag<Match>,
                                const boost::json::value& value)
{
  const auto& object = value.as_object();
  Match match;
  match.expr = boost::json::value_to<std::string>(object.at("expr"));
  return match;
}

namespace
{
  template <typename T>
  T ParseHeaderNameValueAction(const boost::json::value& value,
                               proxypp::rule::http::HeaderOp expected_op,
                               std::string_view action_name_for_description)
  {
    const auto& object = value.as_object();
    const auto op_str = boost::json::value_to<std::string>(object.at("op"));
    const auto op = proxypp::rule::http::HeaderOpFromString(op_str);
    if(op != expected_op)
      {
        throw std::invalid_argument(std::format(
          "invalid HTTP {} op: {}", action_name_for_description, op_str));
      }
    T action;
    action.name = boost::json::value_to<std::string>(object.at("name"));
    action.value = boost::json::value_to<std::string>(object.at("value"));
    return action;
  }
}

proxypp::rule::http::HeaderSetAction
proxypp::rule::http::tag_invoke(boost::json::value_to_tag<HeaderSetAction>,
                                const boost::json::value& value)
{
  return ParseHeaderNameValueAction<HeaderSetAction>(value, HeaderOp::Set,
                                                     "header set");
}

proxypp::rule::http::HeaderAddAction
proxypp::rule::http::tag_invoke(boost::json::value_to_tag<HeaderAddAction>,
                                const boost::json::value& value)
{
  return ParseHeaderNameValueAction<HeaderAddAction>(value, HeaderOp::Add,
                                                     "header add");
}

proxypp::rule::http::HeaderRemoveAction
proxypp::rule::http::tag_invoke(boost::json::value_to_tag<HeaderRemoveAction>,
                                const boost::json::value& value)
{
  const auto& object = value.as_object();
  const auto op_str = boost::json::value_to<std::string>(object.at("op"));
  const auto op = HeaderOpFromString(op_str);
  if(op != HeaderOp::Remove)
    {
      throw std::invalid_argument(
        std::format("invalid HTTP header remove op: {}", op_str));
    }
  HeaderRemoveAction action;
  action.name = boost::json::value_to<std::string>(object.at("name"));
  return action;
}

proxypp::rule::http::HeaderReplaceAction
proxypp::rule::http::tag_invoke(boost::json::value_to_tag<HeaderReplaceAction>,
                                const boost::json::value& value)
{
  return ParseHeaderNameValueAction<HeaderReplaceAction>(
    value, HeaderOp::Replace, "header replace");
}

proxypp::rule::http::Action
proxypp::rule::http::tag_invoke(boost::json::value_to_tag<Action>,
                                const boost::json::value& value)
{
  const auto& obj = value.as_object();
  const auto header_op
    = HeaderOpFromString(boost::json::value_to<std::string>(obj.at("op")));
  switch(header_op)
    {
    case HeaderOp::Add: return boost::json::value_to<HeaderAddAction>(value);
    case HeaderOp::Set: return boost::json::value_to<HeaderSetAction>(value);
    case HeaderOp::Remove:
      return boost::json::value_to<HeaderRemoveAction>(value);
    case HeaderOp::Replace:
      return boost::json::value_to<HeaderReplaceAction>(value);
    case HeaderOp::NotSet: break;
    }
  throw std::invalid_argument("HTTP header action not set ");
}

proxypp::rule::http::Rule
proxypp::rule::http::tag_invoke(boost::json::value_to_tag<Rule>,
                                const boost::json::value& value)
{
  const auto& obj = value.as_object();

  Rule rule;
  rule.name = boost::json::value_to<std::string>(obj.at("name"));
  rule.phase
    = PhaseFromString(boost::json::value_to<std::string>(obj.at("phase")));
  rule.actions = boost::json::value_to<std::vector<Action>>(obj.at("actions"));

  if(const auto* enabled = obj.if_contains("enabled"))
    {
      rule.enabled = enabled->as_bool();
    }

  if(const auto* match = obj.if_contains("match"))
    {
      rule.match = boost::json::value_to<Match>(*match);
    }

  return rule;
}

proxypp::rule::http::Config
proxypp::rule::http::tag_invoke(boost::json::value_to_tag<Config>,
                                const boost::json::value& value)
{
  const auto& obj = value.as_object();
  http::Config http_rules_config;
  if(const auto* rules = obj.if_contains("rules"))
    {
      http_rules_config.rules
        = boost::json::value_to<std::vector<Rule>>(*rules);
    }
  return http_rules_config;
}
