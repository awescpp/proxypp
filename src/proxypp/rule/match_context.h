/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "proxypp/class_utils.h"
#include "proxypp/result.h"
#include <memory>

namespace proxypp::rule
{
  /**
   * MatchContext is a rule-layer wrapper around the underlying script context.
   *
   * Protocol-specific code, such as rule::http context injection, uses this
   * class to expose request/response data to match expressions without
   * depending on the concrete scripting engine implementation.
   */
  class MatchContext
  {
  public:
    ~MatchContext();

    PROXYPP_DISABLE_COPY(MatchContext);

    MatchContext(MatchContext&& other) noexcept;

    MatchContext& operator=(MatchContext&& other) noexcept;

    Result<void> SetString(std::string_view object, std::string_view name,
                           std::string_view value);

    Result<void>
    SetNumber(std::string_view object, std::string_view name, int number);

    Result<void>
    SetBool(std::string_view object, std::string_view name, bool value);

    Result<void>
    SetNestedString(std::string_view object, std::string_view nested_object,
                    std::string_view name, std::string_view value);

  private:
    // allow `RuleEngine` to access the private constructor of `MatchContext`
    friend class RuleEngine;

    class Impl;

    explicit MatchContext(std::unique_ptr<Impl> impl);

    std::unique_ptr<Impl> impl_;
  };
}