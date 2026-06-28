/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "proxypp/rule/match_context.h"
#include "proxypp/script/qjs/context.h"

namespace proxypp::rule
{
  class MatchContext::Impl
  {
  public:
    explicit Impl(script::qjs::Context context) : context_(std::move(context))
    {}

    script::qjs::Context context_;
  };
}