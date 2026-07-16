/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "proxypp/result.h"
#include <memory>
#include <string>
#include <string_view>

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

    /**
     * 获取指定 QuickJS 上下文的全局对象。
     *
     * @param context 要获取全局对象的脚本上下文。
     *
     * @return 成功时返回全局对象；失败时返回错误。
     *
     * @retval Errc::InvalidContext
     *         context 不包含有效的 QuickJS 上下文。
     * @retval Errc::JsInternalError
     *         QuickJS 获取全局对象失败。
     */
    static Result<Value> GlobalObject(Context& context);
    static Result<Value> Undefined(Context& context);
    static Result<Value> Null(Context& context);
    static Result<Value> Bool(Context& context, bool value);
    static Result<Value> Int32(Context& context, std::int32_t value);
    static Result<Value> String(Context& context, std::string_view value);
    static Result<Value> Object(Context& context);
    static Result<Value> Array(Context& context);

    [[nodiscard]]
    bool IsValid() const noexcept;
    [[nodiscard]]
    bool IsException() const noexcept;
    [[nodiscard]]
    bool IsUndefined() const noexcept;
    [[nodiscard]]
    bool IsNull() const noexcept;
    [[nodiscard]]
    bool IsNumber() const noexcept;
    [[nodiscard]]
    bool IsBool() const noexcept;
    [[nodiscard]]
    bool IsString() const noexcept;
    [[nodiscard]]
    bool IsObject() const noexcept;
    [[nodiscard]]
    bool IsArray() const noexcept;

    Result<bool> ToBool() const;
    Result<std::int32_t> ToInt32() const;
    Result<std::string> ToString() const;

    Result<Value> GetProperty(std::string_view name) const;
    Result<void> SetProperty(std::string_view name, Value value);

    Result<void> SetElement(std::uint32_t index, Value value);
    Result<Value> GetElement(std::uint32_t index) const;
    Result<std::uint32_t> ArrayLength() const;

  private:
    friend class detail::ValueAccess;
    class Impl;
    explicit Value(std::unique_ptr<Impl> impl) noexcept;
    std::unique_ptr<Impl> impl_;
  };
}