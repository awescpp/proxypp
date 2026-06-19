/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "proxypp/result.h"
#include <memory>
#include <new>
#include <string>
#include <string_view>
#include <utility>

namespace proxypp::script::qjs
{
  class Context;

  namespace detail
  {
    class ValueAccess;
  }

  class Value
  {
  public:
    ~Value();

    Value(Value&& other) noexcept;
    Value& operator=(Value&& other) noexcept;

    Value(const Value& other) = delete;
    Value& operator=(const Value& other) = delete;

    static Result<Value> Undefined(Context& context);
    static Result<Value> Null(Context& context);
    static Result<Value> Bool(Context& context, bool value);
    static Result<Value> Int32(Context& context, std::int32_t value);
    static Result<Value> String(Context& context, std::string_view value);
    static Result<Value> Object(Context& context);

    bool IsValid() const noexcept;
    bool IsException() const noexcept;
    bool IsUndefined() const noexcept;
    bool IsNull() const noexcept;
    bool IsNumber() const noexcept;
    bool IsString() const noexcept;
    bool IsObject() const noexcept;
    bool IsArray() const noexcept;

    Result<bool> ToBool() const;
    Result<std::int32_t> ToInt32() const;
    Result<std::string> ToString() const;

    Result<Value> GetProperty(std::string_view name) const;
    Result<void> SetProperty(std::string_view name, Value value);

  private:
    friend class detail::ValueAccess;
    class Impl;
    explicit Value(std::unique_ptr<Impl> impl) noexcept;
    std::unique_ptr<Impl> impl_;
  };
}