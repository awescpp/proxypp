/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "proxypp/script/qjs/runtime.h"
#include "proxypp/log/log.h"
#include "proxypp/script/qjs/error.h"
#include <quickjs.h>

namespace
{
  const auto logger = proxypp::log::Get(proxypp::log::Module::qjs);
}

proxypp::Result<proxypp::script::qjs::Runtime>
proxypp::script::qjs::Runtime::Create()
{
  JSRuntime* runtime = JS_NewRuntime();
  if(runtime == nullptr)
    {
      logger->error("failed to create QuickJS runtime");
      return proxypp::Unexpected(Error { Errc::CreateRuntimeFailed });
    }

  logger->trace(R"(QuickJS runtime created: address="{:p}")",
                static_cast<void*>(runtime));

  return Runtime { runtime };
}

JSRuntime* proxypp::script::qjs::Runtime::GetNativeHandle() const noexcept
{
  if(runtime_ == nullptr)
    {
      logger->error("invalid QuickJS runtime: reason=\"nullptr\"");
    }
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

  auto* runtime = runtime_;

  JS_FreeRuntime(runtime_);
  runtime_ = nullptr;

  logger->trace(R"(QuickJS runtime destroyed: address="{:p}")",
                static_cast<void*>(runtime));
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
