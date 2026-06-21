/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once
#include "proxypp/result.h"
#include "proxypp/script/qjs/runtime.h"

struct JSContext;

namespace proxypp::script::qjs
{
  class Context
  {
  public:
    static Result<Context> Create(Runtime& runtime);

    ~Context();

    Context(Context&& other) noexcept;
    Context& operator=(Context&& other) noexcept;

    Context(const Context& other) = delete;
    Context& operator=(const Context& other) = delete;

    JSContext* NativeHandle() noexcept;

  private:
    explicit Context(JSContext* context) noexcept;
    JSContext* context_ = nullptr;
  };
}