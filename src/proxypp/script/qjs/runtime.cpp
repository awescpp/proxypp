/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "proxypp/script/qjs/runtime.h"
#include "proxypp/script/qjs/error.h"
#include <quickjs.h>

proxypp::Result<proxypp::script::qjs::Runtime>
proxypp::script::qjs::Runtime::Create()
{
  JSRuntime* runtime = JS_NewRuntime();
  if(runtime == nullptr)
    {
      return proxypp::Unexpected(Error{Errc::CreateRuntimeFailed});
    }
  return Runtime{runtime};
}

JSRuntime* proxypp::script::qjs::Runtime::NativeHandle() noexcept
{
  return runtime_;
}

proxypp::script::qjs::Runtime::Runtime(JSRuntime* runtime) noexcept
    : runtime_(runtime)
{}

proxypp::script::qjs::Runtime::~Runtime()
{
  if(runtime_ == nullptr)
    {
      return;
    }
  JS_FreeRuntime(runtime_);
  runtime_ = nullptr;
}

proxypp::script::qjs::Runtime::Runtime(Runtime&& other) noexcept
    : runtime_(std::exchange(other.runtime_, nullptr))
{}

proxypp::script::qjs::Runtime&
proxypp::script::qjs::Runtime::operator=(Runtime&& other) noexcept
{
  if(this == &other)
    {
      return *this;
    }
  if(runtime_ != nullptr)
    {
      JS_FreeRuntime(runtime_);
    }
  runtime_ = std::exchange(other.runtime_, nullptr);
  return *this;
}
