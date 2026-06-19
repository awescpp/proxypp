/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once
#include <boost/system/error_code.hpp>

namespace proxypp::script::qjs
{
  enum class Errc
  {
    Ok = 0,
    CreateRuntimeFailed,
    CreateContextFailed,

    EvalFailed,
    JsException,

    ToBoolFailed,
    ToStringFailed,
    ToIntFailed,

    GetPropertyFailed,
    SetPropertyFailed,

    InvalidArgument,
    InternalError
  };

  const boost::system::error_category& GetQjsErrorCategory() noexcept;

  boost::system::error_code make_error_code(Errc errc) noexcept;
}

template <>
struct boost::system::is_error_code_enum<proxypp::script::qjs::Errc>
    : std::true_type
{};