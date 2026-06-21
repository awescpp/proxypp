/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once
#include "proxypp/result.h"

struct JSRuntime;

namespace proxypp::script::qjs
{
  class Runtime
  {
  public:
    static Result<Runtime> Create();

    ~Runtime();

    Runtime(Runtime&& other) noexcept;
    Runtime& operator=(Runtime&& other) noexcept;

    Runtime(const Runtime& other) = delete;
    Runtime& operator=(const Runtime& other) = delete;

    JSRuntime* NativeHandle() noexcept;

  private:
    explicit Runtime(JSRuntime* runtime) noexcept;
    JSRuntime* runtime_ = nullptr;
  };
}