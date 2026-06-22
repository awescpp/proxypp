/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/json.hpp>
#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace proxypp::rule::http
{
  enum class Phase
  {
    NotSet = 0,
    Request,
    Response,
  };

  enum class HeaderOp
  {
    NotSet = 0,
    Set,
    Add,
    Remove,
    Replace,
  };

  struct Match
  {
    std::string expr;
  };

  struct HeaderSetAction
  {
    static constexpr HeaderOp kOp = HeaderOp::Set;
    std::string name;
    std::string value;
  };

  struct HeaderAddAction
  {
    static constexpr HeaderOp kOp = HeaderOp::Add;
    std::string name;
    std::string value;
  };

  struct HeaderRemoveAction
  {
    static constexpr HeaderOp kOp = HeaderOp::Remove;
    std::string name;
  };

  struct HeaderReplaceAction
  {
    static constexpr HeaderOp kOp = HeaderOp::Replace;
    std::string name;
    std::string value;
  };

  using Action = std::variant<HeaderSetAction, HeaderAddAction,
                              HeaderRemoveAction, HeaderReplaceAction>;

  struct Rule
  {
    // JSON key: "name"
    std::string name;

    // JSON key: "enabled"
    // Optional in schema, default true.
    bool enabled = true;

    // JSON key: "phase"
    Phase phase = Phase::NotSet;

    // JSON key: "match"
    // Optional. If omitted, rule matches all contexts in the phase.
    std::optional<Match> match;

    // JSON key: "actions"
    std::vector<Action> actions;
  };

  struct Config
  {
    std::optional<std::vector<Rule>> rules;
  };

  std::ostream& operator<<(std::ostream& os, Phase value);

  std::ostream& operator<<(std::ostream& os, HeaderOp value);

  Phase PhaseFromString(std::string_view value);

  HeaderOp HeaderOpFromString(std::string_view value);

  Match tag_invoke(boost::json::value_to_tag<Match>,
                   const boost::json::value& value);

  HeaderSetAction tag_invoke(boost::json::value_to_tag<HeaderSetAction>,
                             const boost::json::value& value);

  HeaderAddAction tag_invoke(boost::json::value_to_tag<HeaderAddAction>,
                             const boost::json::value& value);

  HeaderRemoveAction tag_invoke(boost::json::value_to_tag<HeaderRemoveAction>,
                                const boost::json::value& value);

  HeaderReplaceAction
  tag_invoke(boost::json::value_to_tag<HeaderReplaceAction>,
             const boost::json::value& value);

  Action tag_invoke(boost::json::value_to_tag<Action>,
                    const boost::json::value& value);

  Rule
  tag_invoke(boost::json::value_to_tag<Rule>, const boost::json::value& value);

  Config tag_invoke(boost::json::value_to_tag<Config>,
                    const boost::json::value& value);

}