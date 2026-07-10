/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/system/error_code.hpp>
#include <string>

namespace proxypp
{

  enum class Errc
  {
    Ok = 0,
    JsonParseFailed,
  };

  const boost::system::error_category& GetErrorCategory() noexcept;

  boost::system::error_code make_error_code(Errc errc) noexcept;

  std::ostream& operator<<(std::ostream& os, Errc errc);

  struct Error
  {
    boost::system::error_code code;
    std::string message;

    explicit Error() = default;

    explicit Error(boost::system::error_code error_code, std::string msg = {});

    explicit operator bool() const noexcept;
  };
}

template <>
struct boost::system::is_error_code_enum<proxypp::Errc> : std::true_type
{};