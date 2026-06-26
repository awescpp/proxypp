/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "model.h"
#include "proxypp/helper.h"
#include <stdexcept>

proxypp::rule::http::data::HeaderNameData
proxypp::rule::http::data::tag_invoke(
  boost::json::value_to_tag<HeaderNameData>, const boost::json::value& value)
{
  HeaderNameData data;
  data.name = boost::json::value_to<std::string>(value.at("name"));
  return data;
}

proxypp::rule::http::data::HeaderNameValueData
proxypp::rule::http::data::tag_invoke(
  boost::json::value_to_tag<HeaderNameValueData>,
  const boost::json::value& value)
{
  HeaderNameValueData data;
  data.name = boost::json::value_to<std::string>(value.at("name"));
  data.value = boost::json::value_to<std::string>(value.at("value"));
  return data;
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

  throw std::invalid_argument(std::format("invalid phase {}", value));
}

std::ostream& proxypp::rule::http::operator<<(std::ostream& os, Phase value)
{
  return helper::WriteEnumName(os, value);
}

proxypp::rule::http::Target
proxypp::rule::http::TargetFromString(std::string_view value)
{
  if(value == "header")
    {
      return Target::Header;
    }
  throw std::invalid_argument(std::format("invalid target {}", value));
}

std::ostream& proxypp::rule::http::operator<<(std::ostream& os, Target value)
{
  return helper::WriteEnumName(os, value);
}

proxypp::rule::http::Action
proxypp::rule::http::tag_invoke(boost::json::value_to_tag<Action>,
                                const boost::json::value& value)
{
  Action action;
  action.op = boost::json::value_to<rule::Op>(value.at("op"));
  action.target = TargetFromString(value.at("target").as_string());
  const auto& data_obj = value.at("data");
  if(action.op == Op::Add || action.op == Op::Set || action.op == Op::Replace)
    {
      action.data = boost::json::value_to<data::HeaderNameValueData>(data_obj);
    }
  else if(action.op == Op::Remove)
    {
      action.data = boost::json::value_to<data::HeaderNameData>(data_obj);
    }
  return action;
}

proxypp::rule::http::Rule
proxypp::rule::http::tag_invoke(boost::json::value_to_tag<Rule>,
                                const boost::json::value& value)
{
  Rule rule;
  rule.name = boost::json::value_to<std::string>(value.at("name"));
  rule.phase = PhaseFromString(value.at("phase").as_string());
  const auto& object = value.as_object();
  if(auto* enabled = object.if_contains("enabled"))
    {
      rule.enabled = boost::json::value_to<bool>(*enabled);
    }
  if(auto* match = object.if_contains("match"))
    {
      rule.match = boost::json::value_to<rule::Match>(*match);
    }
  rule.actions
    = boost::json::value_to<std::vector<Action>>(value.at("actions"));
  return rule;
}

proxypp::rule::http::Config
proxypp::rule::http::tag_invoke(boost::json::value_to_tag<Config>,
                                const boost::json::value& value)
{
  Config config;
  const auto& object = value.as_object();
  if(auto* rules = object.if_contains("rules"))
    {
      config.rules = boost::json::value_to<std::vector<Rule>>(*rules);
    }
  return config;
}
