/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "proxypp/script/qjs/value.h"
#include "proxypp/script/qjs/context.h"
#include "proxypp/script/qjs/detail/exception_message.h"
#include "proxypp/script/qjs/detail/value_access.h"
#include "proxypp/script/qjs/error.h"
#include <quickjs.h>

namespace proxypp::script::qjs
{
  class Value::Impl final
  {
  public:
    // Takes ownership of value.
    // context is borrowed and must outlive this Impl.
    Impl(JSContext& context, JSValue value) noexcept
        : context_(&context), value_(value)
    {}

    ~Impl()
    {
      if(owns_value_ && context_ != nullptr)
        {
          JS_FreeValue(context_, value_);
        }
    }

    Impl(const Impl& other) = delete;
    Impl& operator=(const Impl& other) = delete;
    Impl(Impl&& other) = delete;
    Impl& operator=(Impl&& other) = delete;

    [[nodiscard]]
    bool IsValid() const noexcept
    {
      return context_ != nullptr && owns_value_;
    }

    [[nodiscard]]
    JSContext* Context() const noexcept
    {
      return context_;
    }

    [[nodiscard]]
    JSValue NativeHandle() const noexcept
    {
      return value_;
    }

    // Release ownership of the JSValue from this RAII wrapper
    // After calling Release(), this Impl will no longer JS_FreeValue() it.
    [[nodiscard]]
    JSValue Release() noexcept
    {
      owns_value_ = false;
      context_ = nullptr;
      return std::exchange(value_, JS_UNDEFINED);
    }

  private:
    JSContext* context_ = nullptr; // borrowed, not owned
    JSValue value_ = JS_UNDEFINED;
    bool owns_value_ = true;
  };

  Value::Value(std::unique_ptr<Impl> impl) noexcept : impl_(std::move(impl)) {}

  Value::~Value() = default;

  Value::Value(Value&& other) noexcept = default;

  Value& Value::operator=(Value&& other) noexcept = default;

  Result<Value> Value::GlobalObject(Context& context)
  {
    JSContext* qjs_ctx = context.NativeHandle();
    if(qjs_ctx == nullptr)
      {
        return Unexpected(Error { Errc::InvalidArgument });
      }
    JSValue js_val = JS_GetGlobalObject(qjs_ctx);
    if(JS_IsException(js_val))
      {
        const auto message = detail::GetExceptionMessage(*qjs_ctx);
        JS_FreeValue(qjs_ctx, js_val);
        return Unexpected(Error { Errc::InvalidArgument, message });
      }

    return detail::AdoptValue(*qjs_ctx, js_val);
  }

  Result<Value> Value::Undefined(Context& context)
  {
    JSContext* qjs_ctx = context.NativeHandle();
    if(qjs_ctx == nullptr)
      {
        return Unexpected(Error { Errc::InvalidArgument });
      }
    return detail::AdoptValue(*qjs_ctx, JS_UNDEFINED);
  }

  Result<Value> Value::Null(Context& context)
  {
    JSContext* qjs_ctx = context.NativeHandle();
    if(qjs_ctx == nullptr)
      {
        return Unexpected(Error { Errc::InvalidArgument });
      }
    return detail::AdoptValue(*qjs_ctx, JS_NULL);
  }

  Result<Value> Value::Bool(Context& context, bool value)
  {
    JSContext* qjs_ctx = context.NativeHandle();
    if(qjs_ctx == nullptr)
      {
        return Unexpected(Error { Errc::InvalidArgument });
      }
    return detail::AdoptValue(*qjs_ctx, JS_NewBool(qjs_ctx, value));
  }

  Result<Value> Value::Int32(Context& context, std::int32_t value)
  {
    JSContext* qjs_ctx = context.NativeHandle();
    if(qjs_ctx == nullptr)
      {
        return Unexpected(Error { Errc::InvalidArgument });
      }
    return detail::AdoptValue(*qjs_ctx, JS_NewInt32(qjs_ctx, value));
  }

  Result<Value> Value::String(Context& context, std::string_view value)
  {
    JSContext* qjs_ctx = context.NativeHandle();
    if(qjs_ctx == nullptr)
      {
        return Unexpected(Error { Errc::InvalidArgument });
      }

    const char* c_str = value.empty() ? "" : value.data();

    JSValue js_val = JS_NewStringLen(qjs_ctx, c_str, value.size());
    if(JS_IsException(js_val))
      {
        const std::string message = detail::GetExceptionMessage(*qjs_ctx);
        JS_FreeValue(qjs_ctx, js_val);
        return Unexpected(Error { Errc::InternalError, message });
      }

    return detail::AdoptValue(*qjs_ctx, js_val);
  }

  Result<Value> Value::Object(Context& context)
  {
    JSContext* qjs_ctx = context.NativeHandle();
    if(qjs_ctx == nullptr)
      {
        const Error err { Errc::InvalidArgument };
        return Unexpected(err);
      }

    JSValue js_val = JS_NewObject(qjs_ctx);
    if(JS_IsException(js_val))
      {
        const auto message = detail::GetExceptionMessage(*qjs_ctx);
        JS_FreeValue(qjs_ctx, js_val);
        return Unexpected(Error(Errc::InternalError, message));
      }
    return detail::AdoptValue(*qjs_ctx, js_val);
  }

  bool Value::IsValid() const noexcept
  {
    return impl_ != nullptr && impl_->IsValid();
  }

  bool Value::IsException() const noexcept
  {
    return IsValid() && JS_IsException(impl_->NativeHandle());
  }

  bool Value::IsUndefined() const noexcept
  {
    return IsValid() && JS_IsUndefined(impl_->NativeHandle());
  }

  bool Value::IsNull() const noexcept
  {
    return IsValid() && JS_IsNull(impl_->NativeHandle());
  }

  bool Value::IsNumber() const noexcept
  {
    return IsValid() && JS_IsNumber(impl_->NativeHandle());
  }

  bool Value::IsBool() const noexcept
  {
    return IsValid() && JS_IsBool(impl_->NativeHandle());
  }

  bool Value::IsString() const noexcept
  {
    return IsValid() && JS_IsString(impl_->NativeHandle());
  }

  bool Value::IsObject() const noexcept
  {
    return IsValid() && JS_IsObject(impl_->NativeHandle());
  }

  bool Value::IsArray() const noexcept
  {
    return IsValid() && JS_IsArray(impl_->NativeHandle());
  }

  Result<bool> Value::ToBool() const
  {
    if(!IsValid())
      {
        return Unexpected(Error(Errc::InvalidArgument));
      }
    const int result = JS_ToBool(impl_->Context(), impl_->NativeHandle());
    if(result < 0)
      {
        const auto message = detail::GetExceptionMessage(*impl_->Context());
        return Unexpected(Error(Errc::ToBoolFailed, message));
      }
    return result != 0;
  }

  Result<std::int32_t> Value::ToInt32() const
  {
    if(!IsValid())
      {
        return Unexpected(Error { Errc::InvalidArgument });
      }
    std::int32_t result = 0;
    if(JS_ToInt32(impl_->Context(), &result, impl_->NativeHandle()) < 0)
      {
        const auto message = detail::GetExceptionMessage(*impl_->Context());
        return Unexpected(Error { Errc::ToIntFailed, message });
      }
    return result;
  }

  Result<std::string> Value::ToString() const
  {
    if(!IsValid())
      {
        return Unexpected(Error { Errc::InvalidArgument });
      }
    size_t len = 0;
    const char* text
      = JS_ToCStringLen(impl_->Context(), &len, impl_->NativeHandle());
    if(text == nullptr)
      {
        const auto message = detail::GetExceptionMessage(*impl_->Context());
        return Unexpected(Error { Errc::ToStringFailed, message });
      }

    std::string result { text, len };
    JS_FreeCString(impl_->Context(), text);
    return result;
  }

  Result<Value> Value::GetProperty(std::string_view name) const
  {
    if(!IsValid())
      {
        return Unexpected(Error { Errc::InvalidArgument });
      }

    const std::string property_name { name };
    JSValue property_val = JS_GetPropertyStr(
      impl_->Context(), impl_->NativeHandle(), property_name.c_str());
    if(JS_IsException(property_val))
      {
        const auto message = detail::GetExceptionMessage(*impl_->Context());
        JS_FreeValue(impl_->Context(), property_val);
        return Unexpected(Error { Errc::GetPropertyFailed, message });
      }
    return detail::AdoptValue(*impl_->Context(), property_val);
  }

  Result<void> Value::SetProperty(std::string_view name, Value value)
  {
    if(!IsValid() || !value.IsValid())
      {
        return Unexpected(Error { Errc::InvalidArgument });
      }

    if(impl_->Context() != value.impl_->Context())
      {
        return Unexpected(Error { Errc::InvalidArgument });
      }

    const std::string property_name { name };
    JSValue released_val = value.impl_->Release();

    if(JS_SetPropertyStr(impl_->Context(), impl_->NativeHandle(),
                         property_name.c_str(), released_val)
       < 0)
      {
        const auto message = detail::GetExceptionMessage(*impl_->Context());
        return Unexpected(Error { Errc::SetPropertyFailed, message });
      }

    return {};
  }

}

namespace proxypp::script::qjs::detail
{
  Result<Value> ValueAccess::Adopt(JSContext& context, JSValue value)
  {
    auto impl = std::unique_ptr<Value::Impl>(new(std::nothrow)
                                               Value::Impl(context, value));
    if(!impl)
      {
        JS_FreeValue(&context, value);
        return Unexpected(Error { Errc::InternalError });
      }
    return Value { std::move(impl) };
  }
}
