/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "proxypp/error.h"
#include <magic_enum/magic_enum.hpp>
#include <utility>

namespace proxypp
{
  class ErrorCategory final : public boost::system::error_category
  {
  public:
    const char* name() const noexcept override { return "proxypp::common"; }
    std::string message(int ev) const override
    {
      switch(static_cast<Errc>(ev))
        {
        case Errc::Ok: return "success";
        case Errc::JsonParseFailed: return "parse json failed";
        case Errc::FileNotFound: return "file not found";
        case Errc::FileReadFailed: return "file read failed";
        case Errc::BadFileFormat: return "bad file format";
        case Errc::InvalidArgument: return "invalid argument";
        case Errc::InternalError: return "internal error";
        case Errc::JsonConversionFailed: return "convert json failed";
        case Errc::InvalidJsonSchema: return "invalid json schema";
        case Errc::JsonSchemaValidationFailed:
          return "json schema validation failed";
        }
      return "unknown error";
    }
  };

}

const boost::system::error_category& proxypp::GetErrorCategory() noexcept
{
  static const ErrorCategory category;
  return category;
}

boost::system::error_code proxypp::make_error_code(Errc errc) noexcept
{
  return { static_cast<int>(errc), GetErrorCategory() };
}

std::ostream& proxypp::operator<<(std::ostream& os, Errc errc)
{
  return os << magic_enum::enum_name(errc);
}

proxypp::Error::Error(boost::system::error_code error_code, std::string msg)
    : code_(error_code), message_(std::move(msg))
{
  if(message_.empty() && code_)
    {
      message_ = code_.message();
    }
}

proxypp::Error::operator bool() const noexcept
{
  return static_cast<bool>(code_);
}