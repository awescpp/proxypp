/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "proxypp/rule/http/context_injector.h"
#include "proxypp/rule/match_context.h"
#include "proxypp/script/qjs/object_builder.h"
#include "proxypp/script/qjs/value.h"
#include "request_adapter.h"
#include "response_adapter.h"
#include <type_traits>

namespace qjs = proxypp::script::qjs;

namespace proxypp::rule::http
{
  namespace
  {
    template <typename Adapter>
      requires requires(const Adapter& adapter) { adapter.GetAllHeaders(); }
    Result<qjs::Value>
    BuildHeaderObject(const Adapter& adapter, qjs::Context& script_context)
    {
      const auto headers = adapter.GetAllHeaders();
      auto header_obj_builder = qjs::ObjectBuilder::Create(script_context);
      if(!header_obj_builder.has_value())
        {
          return Unexpected(header_obj_builder.error());
        }
      for(const auto& [name, value] : headers)
        {
          header_obj_builder->SetString(boost::algorithm::to_lower_copy(name),
                                        value);
        }

      return std::move(*header_obj_builder).Build();
    }

    Result<qjs::Value> BuildRequestObject(const RequestAdapter& adapter,
                                          qjs::Context& script_context)
    {
      auto header_object = BuildHeaderObject(adapter, script_context);
      if(!header_object.has_value())
        {
          return Unexpected(header_object.error());
        }

      auto request_obj_builder = qjs::ObjectBuilder::Create(script_context);
      if(!request_obj_builder)
        {
          return Unexpected(request_obj_builder.error());
        }

      request_obj_builder->SetString("method", adapter.Method())
        .SetString("target", adapter.Target())
        .SetInt32("version", static_cast<int>(adapter.Version()))
        .SetObject("headers", std::move(*header_object));

      return std::move(*request_obj_builder).Build();
    }

    Result<qjs::Value> BuildResponseObject(const ResponseAdapter& adapter,
                                           qjs::Context& script_context)
    {
      auto header_object = BuildHeaderObject(adapter, script_context);
      if(!header_object.has_value())
        {
          return Unexpected(header_object.error());
        }

      auto response_obj_builder = qjs::ObjectBuilder::Create(script_context);
      if(!response_obj_builder)
        {
          return Unexpected(response_obj_builder.error());
        }

      response_obj_builder
        ->SetInt32("status", static_cast<int32_t>(adapter.Status()))
        .SetString("reason", adapter.Reason())
        .SetInt32("version", static_cast<int>(adapter.Version()))
        .SetObject("headers", std::move(*header_object));

      return std::move(*response_obj_builder).Build();
    }
  }

  Result<void> InjectRequestContext(MatchContext& context,
                                    const RequestAdapter& request_adapter)
  {
    auto& script_context = context.ScriptContext();

    auto request_obj = BuildRequestObject(request_adapter, script_context);
    if(!request_obj.has_value())
      {
        return Unexpected(request_obj.error());
      }

    auto ctx_obj_builder = qjs::ObjectBuilder::Create(script_context);
    if(!ctx_obj_builder.has_value())
      {
        return Unexpected(ctx_obj_builder.error());
      }

    ctx_obj_builder->SetString("phase", "request")
      .SetObject("request", std::move(*request_obj));

    auto ctx_obj = std::move(*ctx_obj_builder).Build();
    if(!ctx_obj.has_value())
      {
        return Unexpected(ctx_obj.error());
      }

    return context.AddObject("ctx", std::move(*ctx_obj));
  }

  Result<void> InjectResponseContext(MatchContext& context,
                                     const RequestAdapter& request_adapter,
                                     const ResponseAdapter& response_adapter)
  {
    auto& script_context = context.ScriptContext();

    auto request_obj = BuildRequestObject(request_adapter, script_context);
    if(!request_obj.has_value())
      {
        return Unexpected(request_obj.error());
      }

    auto response_obj = BuildResponseObject(response_adapter, script_context);
    if(!response_obj)
      {
        return Unexpected(response_obj.error());
      }

    auto ctx_obj_builder = qjs::ObjectBuilder::Create(script_context);
    if(!ctx_obj_builder.has_value())
      {
        return Unexpected(ctx_obj_builder.error());
      }

    ctx_obj_builder->SetString("phase", "response")
      .SetObject("request", std::move(*request_obj))
      .SetObject("response", std::move(*response_obj));
    auto ctx_obj = std::move(*ctx_obj_builder).Build();
    if(!ctx_obj.has_value())
      {
        return Unexpected(ctx_obj.error());
      }

    return context.AddObject("ctx", std::move(*ctx_obj));
  }

}
