/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "proxypp/script/qjs/error.h"
#include "iostream"
#include <magic_enum/magic_enum.hpp>

namespace proxypp::script::qjs
{
  class QjsErrorCategory final : public boost::system::error_category
  {
  public:
    const char* name() const noexcept override { return "proxypp.qjs"; }

    std::string message(int ev) const override
    {
      switch(static_cast<Errc>(ev))
        {
        case Errc::Ok: return "success";
        case Errc::CreateRuntimeFailed:
          return "failed to create QuickJS runtime";
        case Errc::CreateContextFailed:
          return "failed to create QuickJS context";
        case Errc::InvalidRuntime: return "invalid runtime";
        case Errc::InvalidContext: return "invalid context";
        case Errc::InvalidValue: return "invalid value";
        case Errc::ContextMismatch: return "context mismatch";
        case Errc::ConvertValueFailed: return "convert value failed";
        case Errc::ExecuteScriptFailed: return "execute script failed";
        case Errc::GetPropertyFailed:
          return "failed to get JavaScript property";
        case Errc::SetPropertyFailed:
          return "failed to set JavaScript property";
        case Errc::GetElementFailed: return "get element from array failed";
        case Errc::SetElementFailed: return "set element to array failed";
        case Errc::JsInternalError: return "javascript runtime internal error";
        }
      return "unknown QuickJS error";
    }
  };

  const boost::system::error_category& GetQjsErrorCategory() noexcept
  {
    static const QjsErrorCategory category;
    return category;
  }

  boost::system::error_code make_error_code(Errc errc) noexcept
  {
    return { static_cast<int>(errc), GetQjsErrorCategory() };
  }

  std::ostream& operator<<(std::ostream& os, Errc errc)
  {
    os << magic_enum::enum_name(errc);
    return os;
  }
}