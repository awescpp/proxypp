/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "proxypp/result.h"
#include "proxypp/script/qjs/context.h"
#include "proxypp/script/qjs/value.h"
#include <optional>

namespace proxypp::script::qjs
{
  class ObjectBuilder
  {
  public:
    static Result<ObjectBuilder> Create(Context& context);

    ObjectBuilder& SetString(std::string_view name, std::string_view value);

    ObjectBuilder& SetBool(std::string_view name, bool value);

    ObjectBuilder& SetInt32(std::string_view name, int value);

    ObjectBuilder& SetNull(std::string_view name);

    ObjectBuilder& SetObject(std::string_view name, Value object);

    ObjectBuilder& SetValue(std::string_view name, Value value);

    Result<Value> Build() &&;

  private:

    explicit ObjectBuilder(Context& context, Value object);

    void SetProperty(std::string_view name, Value value);

    Context& context_;
    Value object_;
    std::optional<Error> error_;
  };
}