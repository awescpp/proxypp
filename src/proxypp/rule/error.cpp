/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "proxypp/rule/error.h"
#include <magic_enum/magic_enum.hpp>
#include <string>

namespace proxypp::rule
{
  class RuleErrorCategory final : public boost::system::error_category
  {
  public:
    const char* name() const noexcept override { return "proxypp.rule"; }
    std::string message(int ev) const override
    {
      switch(static_cast<Errc>(ev))
        {
        case Errc::Ok: return "success";
        case Errc::InvalidRuleSet: return "invalid rule set";
        case Errc::InvalidRule: return "invalid rule";
        case Errc::UnsupportedHttpPhase: return "unsupported HTTP phase";
        case Errc::RuntimeCreationFailed:
          return "script runtime creation failed";
        case Errc::RuleEngineInitializationFailed:
          return "rule engine initialization failed";
        case Errc::ContextCreationFailed:
          return "script context creation failed";
        case Errc::ContextInjectionFailed:
          return "rule context injection failed";
        case Errc::InvalidMatchExpression: return "invalid match expression";
        case Errc::MatchEvaluationFailed:
          return "match expression evaluation failed";
        case Errc::MatchResultNotBoolean:
          return "match expression result is not boolean";
        case Errc::InvalidAction: return "invalid rule action";
        case Errc::UnsupportedAction: return "unsupported rule action";
        case Errc::ActionExecutionFailed:
          return "rule action execution failed";
        }
      return "unknown rule error";
    }
  };
}

const boost::system::error_category&
proxypp::rule::GetRuleErrorCategory() noexcept
{
  static const RuleErrorCategory category;
  return category;
}

boost::system::error_code proxypp::rule::make_error_code(Errc errc) noexcept
{
  return { static_cast<int>(errc), GetRuleErrorCategory() };
}

std::ostream& proxypp::rule::operator<<(std::ostream& os, Errc errc)
{
  os << magic_enum::enum_name(errc);
  return os;
}
