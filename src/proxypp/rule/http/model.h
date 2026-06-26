/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "proxypp/rule/match.h"
#include "proxypp/rule/op.h"
#include <boost/json.hpp>

namespace proxypp::rule::http
{
  namespace data
  {
    struct HeaderNameData
    {
      std::string name;
    };

    HeaderNameData tag_invoke(boost::json::value_to_tag<HeaderNameData>,
                              const boost::json::value& value);

    struct HeaderNameValueData
    {
      std::string name;
      std::string value;
    };

    HeaderNameValueData
    tag_invoke(boost::json::value_to_tag<HeaderNameValueData>,
               const boost::json::value& value);
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

  Action tag_invoke(boost::json::value_to_tag<Action>,
                    const boost::json::value& value);

  struct Rule
  {
    std::string name;
    bool enabled = true;
    Phase phase = Phase::NotSet;
    std::optional<rule::Match> match;
    std::vector<Action> actions;
  };

  Rule
  tag_invoke(boost::json::value_to_tag<Rule>, const boost::json::value& value);

  struct Config
  {
    std::optional<std::vector<Rule>> rules;
  };

  Config tag_invoke(boost::json::value_to_tag<Config>,
                    const boost::json::value& value);

}