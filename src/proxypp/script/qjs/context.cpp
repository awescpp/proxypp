/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "proxypp/script/qjs/context.h"
#include "proxypp/script/qjs/error.h"
#include <quickjs.h>

proxypp::Result<proxypp::script::qjs::Context>
proxypp::script::qjs::Context::Create(Runtime& runtime)
{
  JSContext* context = JS_NewContext(runtime.RawPtr());
  if(context == nullptr)
    {
      return proxypp::Unexpected(proxypp::Error{Errc::CreateContextFailed});
    }
  return Context{context};
}

proxypp::script::qjs::Context::~Context()
{
  if(context_ == nullptr)
    {
      return;
    }
  JS_FreeContext(context_);
  context_ = nullptr;
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

JSContext* proxypp::script::qjs::Context::RawPtr() noexcept
{
  return context_;
}

proxypp::script::qjs::Context::Context(JSContext* context) noexcept
    : context_(context)
{}