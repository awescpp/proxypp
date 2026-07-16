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
        case Errc::InvalidRule: return "invalid rule";
        case Errc::RuleEngineInitializationFailed:
          return "rule engine initialization failed";
        case Errc::RuleContextPreparationFailed:
          return "rule context preparation failed";
        case Errc::InvalidMatchExpression: return "invalid match expression";
        case Errc::MatchEvaluationFailed:
          return "match expression evaluation failed";
        case Errc::MatchResultNotBoolean:
          return "match expression result is not boolean";
        case Errc::InvalidAction: return "invalid rule action";
        case Errc::ActionExecutionFailed: return "action execution failed";
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
