/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "proxypp/script/qjs/value.h"
#include "proxypp/log/log.h"
#include "proxypp/script/qjs/context.h"
#include "proxypp/script/qjs/detail/exception_message.h"
#include "proxypp/script/qjs/detail/value_access.h"
#include "proxypp/script/qjs/error.h"
#include <format>
#include <quickjs.h>

namespace proxypp::script::qjs
{

  namespace
  {
    const auto logger = log::Get(log::Module::qjs);
  }

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

  decltype(Unexpected(std::declval<Error>()))
  Value::HandleInvalidValue(std::string_view message, std::string_view reason)
  {
    logger->error(R"({}, reason="{}")", message, reason);
    return Unexpected(Error { Errc::InvalidValue, std::string { reason } });
  }

  decltype(Unexpected(std::declval<Error>()))
  Value::HandleContextMismatch(std::string_view message)
  {
    logger->error(R"({}, reason="QuickJS Context mismatch")", message);
    return Unexpected(Error(Errc::ContextMismatch));
  }

  Result<Value> Value::GlobalObject(Context& context)
  {
    JSContext* qjs_ctx = context.GetNativeHandle();
    if(qjs_ctx == nullptr)
      {
        return Unexpected(Error { Errc::InvalidContext });
      }

    JSValue js_val = JS_GetGlobalObject(qjs_ctx);
    if(JS_IsException(js_val))
      {
        const auto message = detail::GetExceptionMessage(*qjs_ctx);
        JS_FreeValue(qjs_ctx, js_val);

        logger->error(R"(failed to get QuickJS global object: error="{}")",
                      message);

        return Unexpected(Error { Errc::JsInternalError, message });
      }

    return detail::AdoptValue(*qjs_ctx, js_val);
  }

  Result<Value> Value::Undefined(Context& context)
  {
    JSContext* qjs_ctx = context.GetNativeHandle();
    if(qjs_ctx == nullptr)
      {
        return Unexpected(Error { Errc::InvalidContext });
      }
    return detail::AdoptValue(*qjs_ctx, JS_UNDEFINED);
  }

  Result<Value> Value::Null(Context& context)
  {
    JSContext* qjs_ctx = context.GetNativeHandle();
    if(qjs_ctx == nullptr)
      {
        return Unexpected(Error { Errc::InvalidContext });
      }
    return detail::AdoptValue(*qjs_ctx, JS_NULL);
  }

  Result<Value> Value::Bool(Context& context, bool value)
  {
    JSContext* qjs_ctx = context.GetNativeHandle();
    if(qjs_ctx == nullptr)
      {
        return Unexpected(Error { Errc::InvalidContext });
      }
    return detail::AdoptValue(*qjs_ctx, JS_NewBool(qjs_ctx, value));
  }

  Result<Value> Value::Int32(Context& context, std::int32_t value)
  {
    JSContext* qjs_ctx = context.GetNativeHandle();
    if(qjs_ctx == nullptr)
      {
        return Unexpected(Error { Errc::InvalidContext });
      }
    return detail::AdoptValue(*qjs_ctx, JS_NewInt32(qjs_ctx, value));
  }

  Result<Value> Value::String(Context& context, std::string_view value)
  {
    JSContext* qjs_ctx = context.GetNativeHandle();
    if(qjs_ctx == nullptr)
      {
        return Unexpected(Error { Errc::InvalidContext });
      }

    const char* c_str = value.empty() ? "" : value.data();

    JSValue js_val = JS_NewStringLen(qjs_ctx, c_str, value.size());
    if(JS_IsException(js_val))
      {
        const std::string message = detail::GetExceptionMessage(*qjs_ctx);
        JS_FreeValue(qjs_ctx, js_val);

        logger->error(
          R"(failed to create String value: length={}, error="{}")",
          value.size(), message);

        return Unexpected(Error { Errc::JsInternalError, message });
      }

    return detail::AdoptValue(*qjs_ctx, js_val);
  }

  Result<Value> Value::Object(Context& context)
  {
    JSContext* qjs_ctx = context.GetNativeHandle();
    if(qjs_ctx == nullptr)
      {
        return Unexpected(Error { Errc::InvalidContext });
      }

    JSValue js_val = JS_NewObject(qjs_ctx);
    if(JS_IsException(js_val))
      {
        const auto message = detail::GetExceptionMessage(*qjs_ctx);
        JS_FreeValue(qjs_ctx, js_val);

        logger->error(R"(failed to create Object: error="{}")", message);

        return Unexpected(Error(Errc::JsInternalError, message));
      }
    return detail::AdoptValue(*qjs_ctx, js_val);
  }

  Result<Value> Value::Array(Context& context)
  {
    JSContext* qjs_ctx = context.GetNativeHandle();
    if(qjs_ctx == nullptr)
      {
        return Unexpected(Error(Errc::InvalidContext));
      }
    JSValue array_val = JS_NewArray(qjs_ctx);
    if(JS_IsException(array_val))
      {
        const auto message = detail::GetExceptionMessage(*qjs_ctx);
        JS_FreeValue(qjs_ctx, array_val);

        logger->error(R"(failed to create Array: error="{}")", message);

        return Unexpected(Error(Errc::JsInternalError, message));
      }
    return detail::AdoptValue(*qjs_ctx, array_val);
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
        return HandleInvalidValue("failed to convert value to Boolean",
                                  "value is invalid");
      }
    const int result = JS_ToBool(impl_->Context(), impl_->NativeHandle());
    if(result < 0)
      {
        const auto message = detail::GetExceptionMessage(*impl_->Context());

        logger->error(R"(failed to convert value to Boolean: error="{}")",
                      message);

        return Unexpected(Error(Errc::ConvertValueFailed, message));
      }
    return result != 0;
  }

  Result<std::int32_t> Value::ToInt32() const
  {
    if(!IsValid())
      {
        return HandleInvalidValue("failed to convert value to Int32",
                                  "value is invalid");
      }
    std::int32_t result = 0;
    if(JS_ToInt32(impl_->Context(), &result, impl_->NativeHandle()) < 0)
      {
        const auto message = detail::GetExceptionMessage(*impl_->Context());

        logger->error(R"(failed to convert value to Int32: error="{}")",
                      message);

        return Unexpected(Error { Errc::ConvertValueFailed, message });
      }
    return result;
  }

  Result<std::string> Value::ToString() const
  {
    if(!IsValid())
      {
        return HandleInvalidValue("failed to convert value to String",
                                  "value is invalid");
      }

    size_t len = 0;
    const char* text
      = JS_ToCStringLen(impl_->Context(), &len, impl_->NativeHandle());
    if(text == nullptr)
      {
        const auto message = detail::GetExceptionMessage(*impl_->Context());

        logger->error(R"(failed to convert value to String: error="{}")",
                      message);

        return Unexpected(Error { Errc::ConvertValueFailed, message });
      }

    std::string result { text, len };
    JS_FreeCString(impl_->Context(), text);
    return result;
  }

  Result<Value> Value::GetProperty(std::string_view name) const
  {
    if(!IsValid())
      {
        return HandleInvalidValue(
          std::format(R"(failed to get propery: property_name="{}")", name),
          "target is invalid");
      }

    const std::string property_name { name };
    JSValue property_val = JS_GetPropertyStr(
      impl_->Context(), impl_->NativeHandle(), property_name.c_str());
    if(JS_IsException(property_val))
      {
        const auto message = detail::GetExceptionMessage(*impl_->Context());

        JS_FreeValue(impl_->Context(), property_val);

        logger->error(R"(failed to get property: name="{}", error="{}")", name,
                      message);

        return Unexpected(Error { Errc::GetPropertyFailed, message });
      }
    return detail::AdoptValue(*impl_->Context(), property_val);
  }

  Result<void> Value::SetProperty(std::string_view name, Value value)
  {
    if(!IsValid())
      {
        return HandleInvalidValue(
          std::format(R"(failed to set property: property_name="{}")", name),
          "target is invalid");
      }

    if(!IsObject())
      {
        return HandleInvalidValue(
          std::format(R"(failed to set property: property_name="{}")", name),
          "target is not an object");
      }

    if(!value.IsValid())
      {
        return HandleInvalidValue(
          std::format(R"(failed to set property: property_name="{}")", name),
          "property value is invalid");
      }

    if(impl_->Context() != value.impl_->Context())
      {
        return HandleContextMismatch(
          std::format(R"(failed to set property: property_name="{}")", name));
      }

    const std::string property_name { name };
    JSValue released_val = value.impl_->Release();

    if(JS_SetPropertyStr(impl_->Context(), impl_->NativeHandle(),
                         property_name.c_str(), released_val)
       < 0)
      {
        const auto message = detail::GetExceptionMessage(*impl_->Context());

        logger->error(
          R"(failed to set property: property_name="{}", error="{}")", name,
          message);

        return Unexpected(Error { Errc::SetPropertyFailed, message });
      }

    return {};
  }

  Result<void> Value::SetElement(std::uint32_t index, Value value)
  {
    if(!IsValid())
      {
        return HandleInvalidValue(
          std::format(R"(failed to set element to array: index={})", index),
          "target is invalid");
      }

    if(!IsArray())
      {
        return HandleInvalidValue(
          std::format(R"(failed to set element to array: index={})", index),
          "target is not an array");
      }

    if(!value.IsValid())
      {
        return HandleInvalidValue(
          std::format(R"(failed to set element to array: index={})", index),
          "value to set is invalid");
      }

    if(impl_->Context() != value.impl_->Context())
      {
        return HandleContextMismatch(
          std::format("failed to set element to array: index={}", index));
      }

    JSValue released_val = value.impl_->Release();

    if(JS_SetPropertyUint32(impl_->Context(), impl_->NativeHandle(), index,
                            released_val)
       < 0)
      {
        const auto message = detail::GetExceptionMessage(*impl_->Context());

        logger->error(
          R"(failed to set element to array: index={}, error="{}")", index,
          message);

        return Unexpected(Error { Errc::SetElementFailed, message });
      }
    return {};
  }

  Result<Value> Value::GetElement(std::uint32_t index) const
  {
    if(!IsValid())
      {
        return HandleInvalidValue(
          std::format("failed to get element from array: index={}", index),
          "target is invalid");
      }

    if(!IsArray())
      {
        return HandleInvalidValue(
          std::format("failed to get element from array: index={}", index),
          "target is not an array");
      }

    JSValue js_value
      = JS_GetPropertyUint32(impl_->Context(), impl_->NativeHandle(), index);
    if(JS_IsException(js_value))
      {
        const auto message = detail::GetExceptionMessage(*impl_->Context());
        JS_FreeValue(impl_->Context(), js_value);

        logger->error(
          R"(failed to get element from array: index={}, error="{}")", index,
          message);

        return Unexpected(Error { Errc::GetElementFailed, message });
      }
    return detail::AdoptValue(*impl_->Context(), js_value);
  }

  Result<std::uint32_t> Value::ArrayLength() const
  {
    if(!IsValid())
      {
        return HandleInvalidValue("failed to get array length",
                                  "target is invalid");
      }

    if(!IsArray())
      {
        return HandleInvalidValue("failed to get array length",
                                  "target is not an array");
      }

    JSValue js_value
      = JS_GetPropertyStr(impl_->Context(), impl_->NativeHandle(), "length");

    if(JS_IsException(js_value))
      {
        const auto message = detail::GetExceptionMessage(*impl_->Context());
        JS_FreeValue(impl_->Context(), js_value);

        logger->error(R"(failed to get array length: error="{}")", message);

        return Unexpected(Error { Errc::GetPropertyFailed, message });
      }

    auto adopted_value = detail::AdoptValue(*impl_->Context(), js_value);

    if(!adopted_value)
      {
        return Unexpected(adopted_value.error());
      }

    if(!adopted_value->IsNumber())
      {
        logger->error(
          R"(failed to get array length: reason="array length is not a number")");

        return Unexpected(
          Error(Errc::JsInternalError, "array length is not a number"));
      }

    const auto length_result = adopted_value->ToInt32();
    if(!length_result)
      {
        return Unexpected(length_result.error());
      }

    return *length_result;
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

        logger->error(R"(failed to adopt QuickJS value)");

        return Unexpected(
          Error { proxypp::Errc::InternalError, "failed to adopt JS value" });
      }
    return Value { std::move(impl) };
  }
}
