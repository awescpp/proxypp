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
        case Errc::EvalFailed:
          return "failed to evaluate JavaScript expression";
        case Errc::JsException: return "JavaScript exception";
        case Errc::ToBoolFailed:
          return "failed to convert JavaScript value to bool";
        case Errc::ToStringFailed:
          return "failed to convert JavaScript value to string";
        case Errc::ToIntFailed:
          return "failed to convert JavaScript value to int";
        case Errc::GetPropertyFailed:
          return "failed to get JavaScript property";
        case Errc::SetPropertyFailed:
          return "failed to set JavaScript property";
        case Errc::InvalidArgument: return "invalid argument";
        case Errc::InternalError: return "internal QuickJS wrapper error";
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