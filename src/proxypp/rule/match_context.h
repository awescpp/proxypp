/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "proxypp/class_utils.h"
#include "proxypp/result.h"
#include "proxypp/script/qjs/context.h"
#include "proxypp/script/qjs/value.h"
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
    static Result<MatchContext> Create(script::qjs::Context context);

    ~MatchContext();

    PROXYPP_DISABLE_COPY(MatchContext);

    MatchContext(MatchContext&& other) noexcept;

    MatchContext& operator=(MatchContext&& other) noexcept;

    script::qjs::Context& ScriptContext() noexcept;

    Result<void> AddObject(std::string_view name, script::qjs::Value object);

    Result<void>
    AddGlobalValue(std::string_view name, script::qjs::Value value);

  private:

    class Impl;

    explicit MatchContext(std::unique_ptr<Impl> impl);

    std::unique_ptr<Impl> impl_;
  };
}