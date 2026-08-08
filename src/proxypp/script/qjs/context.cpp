/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "proxypp/script/qjs/context.h"
#include "proxypp/log/log.h"
#include "proxypp/script/qjs/error.h"
#include <quickjs.h>
#include <utility>

namespace
{
  const auto logger = proxypp::log::Get(proxypp::log::Module::qjs);
}

proxypp::Result<proxypp::script::qjs::Context>
proxypp::script::qjs::Context::Create(Runtime& runtime)
{
  JSContext* context = JS_NewContext(runtime.GetNativeHandle());
  if(context == nullptr)
    {
      logger->error("failed to create QuickJS context");
      return proxypp::Unexpected(proxypp::Error { Errc::CreateContextFailed });
    }

  logger->trace(R"(QuickJS context created: address="{:p}")",
                static_cast<void*>(context));

  return Context { context };
}

proxypp::script::qjs::Context::~Context()
{
  if(context_ == nullptr)
    {
      return;
    }

  auto* context = context_;

  JS_FreeContext(context_);

  context_ = nullptr;

  logger->trace(R"(QuickJS context destroyed: address="{:p}")",
                static_cast<void*>(context));
}

proxypp::script::qjs::Context::Context(Context&& other) noexcept
    : context_(std::exchange(other.context_, nullptr))
{}

proxypp::script::qjs::Context&
proxypp::script::qjs::Context::operator=(Context&& other) noexcept
{
  if(this == &other)
    {
      return *this;
    }

  if(context_ != nullptr)
    {
      JS_FreeContext(context_);
      context_ = nullptr;
    }

  context_ = std::exchange(other.context_, nullptr);

  return *this;
}

JSContext* proxypp::script::qjs::Context::GetNativeHandle() const noexcept
{
  if(context_ == nullptr)
    {
      logger->error("invalid QuickJS context: reason=\"nullptr\"");
    }
  return context_;
}

proxypp::script::qjs::Context::Context(JSContext* context) noexcept
    : context_(context)
{}
