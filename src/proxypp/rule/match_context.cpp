/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "proxypp/rule/match_context.h"
#include "proxypp/rule/detail/match_context_impl.h"
#include "proxypp/script/qjs/value.h"

namespace proxypp::rule
{
  MatchContext::MatchContext(std::unique_ptr<Impl> impl)
      : impl_(std::move(impl))
  {}

  MatchContext::~MatchContext() = default;

  MatchContext::MatchContext(MatchContext&& other) noexcept = default;

  MatchContext&
  MatchContext::operator=(MatchContext&& other) noexcept = default;

  Result<void>
  MatchContext::SetString(std::string_view object, std::string_view name,
                          std::string_view value)
  {
    return {};
  }

  Result<void> MatchContext::SetNumber(std::string_view object,
                                       std::string_view name, int number)
  {
    return {};
  }

  Result<void> MatchContext::SetBool(std::string_view object,
                                     std::string_view name, bool value)
  {
    return {};
  }

  Result<void>
  MatchContext::SetNestedString(std::string_view object,
                                std::string_view nested_object,
                                std::string_view name, std::string_view value)
  {
    return {};
  }

}