/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "proxypp/script/qjs/object_builder.h"
#include "proxypp/http/body_info.h"
#include "proxypp/script/qjs/error.h"
#include <format>

proxypp::Result<proxypp::script::qjs::ObjectBuilder>
proxypp::script::qjs::ObjectBuilder::Create(Context& context)
{
  auto object = qjs::Value::Object(context);
  if(!object.has_value())
    {
      return Unexpected(Error(Errc::InternalError, object.error().message));
    }
  return ObjectBuilder { context, std::move(*object) };
}

proxypp::script::qjs::ObjectBuilder&
proxypp::script::qjs::ObjectBuilder::SetString(std::string_view name,
                                               std::string_view value)
{
  if(error_.has_value())
    {
      return *this;
    };
  auto js_value = qjs::Value::String(context_, value);
  if(!js_value.has_value())
    {
      error_ = Error(Errc::InvalidArgument, js_value.error().message);
      return *this;
    }
  SetProperty(name, std::move(*js_value));
  return *this;
}

proxypp::script::qjs::ObjectBuilder&
proxypp::script::qjs::ObjectBuilder::SetBool(std::string_view name, bool value)
{
  if(error_.has_value())
    {
      return *this;
    };
  auto js_value = qjs::Value::Bool(context_, value);
  if(!js_value.has_value())
    {
      error_ = Error(Errc::InvalidArgument, js_value.error().message);
      return *this;
    }
  SetProperty(name, std::move(*js_value));
  return *this;
}

proxypp::script::qjs::ObjectBuilder&
proxypp::script::qjs::ObjectBuilder::SetInt32(std::string_view name, int value)
{
  if(error_.has_value())
    {
      return *this;
    };
  auto js_value = qjs::Value::Int32(context_, value);
  if(!js_value.has_value())
    {
      error_ = Error(Errc::InvalidArgument, js_value.error().message);
      return *this;
    }
  SetProperty(name, std::move(*js_value));
  return *this;
}

proxypp::script::qjs::ObjectBuilder&
proxypp::script::qjs::ObjectBuilder::SetNull(std::string_view name)
{
  if(error_.has_value())
    {
      return *this;
    };
  auto js_value = qjs::Value::Null(context_);
  if(!js_value.has_value())
    {
      error_ = Error(Errc::InternalError, js_value.error().message);
      return *this;
    }
  SetProperty(name, std::move(*js_value));
  return *this;
}

proxypp::script::qjs::ObjectBuilder&
proxypp::script::qjs::ObjectBuilder::SetObject(std::string_view name,
                                               Value object)
{
  if(error_.has_value())
    {
      return *this;
    };

  if(!object.IsValid() || !object.IsObject())
    {
      error_ = Error(Errc::InvalidArgument, "value is not an object");
    }

  SetProperty(name, std::move(object));

  return *this;
}

proxypp::script::qjs::ObjectBuilder&
proxypp::script::qjs::ObjectBuilder::SetValue(std::string_view name,
                                              Value value)
{
  if(error_.has_value())
    {
      return *this;
    };

  if(!value.IsValid())
    {
      error_ = Error(Errc::InvalidArgument, "value is invalid");
    }

  SetProperty(name, std::move(value));

  return *this;
}

proxypp::Result<proxypp::script::qjs::Value>
proxypp::script::qjs::ObjectBuilder::Build() &&
{
  if(error_.has_value())
    {
      return Unexpected(*error_);
    }
  return std::move(object_);
}

proxypp::script::qjs::ObjectBuilder::ObjectBuilder(Context& context,
                                                   Value object)
    : context_(context), object_(std::move(object))
{}

void proxypp::script::qjs::ObjectBuilder::SetProperty(std::string_view name,
                                                      Value value)
{
  if(const auto result = object_.SetProperty(name, std::move(value));
     !result.has_value())
    {
      error_ = Error(Errc::SetPropertyFailed,
                     std::format("set property '{}' failed, {}", name,
                                 result.error().message));
    }
}