/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/system/error_code.hpp>
#include <iosfwd>

namespace proxypp::rule
{
  enum class Errc
  {
    Ok = 0,

    InvalidRule,

    RuleEngineInitializationFailed,
    RuleContextPreparationFailed,

    InvalidMatchExpression,
    MatchEvaluationFailed,
    MatchResultNotBoolean,

    InvalidAction,
    ActionExecutionFailed,
  };

  const boost::system::error_category& GetRuleErrorCategory() noexcept;

  boost::system::error_code make_error_code(Errc errc) noexcept;

  std::ostream& operator<<(std::ostream& os, Errc errc);

}

template <>
struct boost::system::is_error_code_enum<proxypp::rule::Errc> : std::true_type
{};