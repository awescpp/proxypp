/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once
#include <boost/system/error_code.hpp>
#include <iostream>

namespace proxypp::script::qjs
{
  enum class Errc
  {
    Ok = 0,
    CreateRuntimeFailed,
    CreateContextFailed,

    InvalidRuntime,
    InvalidContext,
    InvalidValue,
    ContextMismatch,

    ConvertValueFailed,
    ExecuteScriptFailed,

    GetPropertyFailed,
    SetPropertyFailed,

    GetElementFailed,
    SetElementFailed,

    JsInternalError
  };

  const boost::system::error_category& GetQjsErrorCategory() noexcept;

  boost::system::error_code make_error_code(Errc errc) noexcept;

  std::ostream& operator<<(std::ostream& os, Errc errc);
}

template <>
struct boost::system::is_error_code_enum<proxypp::script::qjs::Errc>
    : std::true_type
{};