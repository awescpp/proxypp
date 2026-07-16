/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/system/error_code.hpp>
#include <string>
#include <string_view>

namespace proxypp
{

  enum class Errc
  {
    Ok = 0,

    InternalError,
    InvalidArgument,

    FileNotFound,
    FileReadFailed,
    BadFileFormat,

    JsonParseFailed,
    JsonConversionFailed,
    InvalidJsonSchema,
    JsonSchemaValidationFailed,
  };

  const boost::system::error_category& GetErrorCategory() noexcept;

  boost::system::error_code make_error_code(Errc errc) noexcept;

  std::ostream& operator<<(std::ostream& os, Errc errc);

  class Error
  {
  public:
    Error() = default;

    explicit Error(boost::system::error_code error_code, std::string msg = {});

    explicit operator bool() const noexcept;

    [[nodiscard]] boost::system::error_code code() const noexcept
    {
      return code_;
    }

    [[nodiscard]] std::string message() const noexcept { return message_; }

  private:
    boost::system::error_code code_;
    std::string message_;
  };
}

template <>
struct boost::system::is_error_code_enum<proxypp::Errc> : std::true_type
{};