/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "proxypp/rule/http/action_executor.h"
#include "proxypp/rule/error.h"
#include "proxypp/rule/op.h"
#include "request_adapter.h"
#include "response_adapter.h"
#include <unordered_set>

namespace
{
  const std::unordered_set<std::string> kUnsupportedHeaders = {
    // Body framing
    "content-length", "transfer-encoding", "te", "trailer",

    // Hop-by-hop / connection management
    "connection", "keep-alive", "proxy-connection", "upgrade",

    // routing
    "host",

    // Body representation / partial content
    "proxy-authorization", "proxy-authenticate", "content-encoding",
    "content-range", "range", "if-range", "accept-ranges",

    // Request body flow control
    "expect",

    // Auth
    "authorization", "www-authenticate", "authentication-info",
    "proxy-authentication-info",

    // Proxy-managed metadata
    "via", "x-forwarded-for", "x-forwarded-host", "x-forwarded-proto",
    "x-real-ip"
  };
}

proxypp::Result<void>
proxypp::rule::http::ExecuteRequestAction(const Action& action,
                                          RequestAdapter& request_adapter)
{
  if(action.target != Target::Header)
    {
      return Unexpected(
        Error { Errc::UnsupportedAction, "unsupported target" });
    }

  if(std::holds_alternative<data::HeaderNameData>(action.data))
    {
      const auto& data = std::get<data::HeaderNameData>(action.data);
      if(kUnsupportedHeaders.contains(
           boost::algorithm::to_lower_copy(data.name)))
        {
          return Unexpected(
            Error { Errc::InvalidAction,
                    std::format("unsupported header {}", data.name) });
        }
    }

  if(std::holds_alternative<data::HeaderNameValueData>(action.data))
    {
      const auto& data = std::get<data::HeaderNameValueData>(action.data);
      if(kUnsupportedHeaders.contains(
           boost::algorithm::to_lower_copy(data.name)))
        {
          return Unexpected(
            Error { Errc::InvalidAction,
                    std::format("unsupported header {}", data.name) });
        }
    }

  if(action.op == Op::Add)
    {
      if(std::holds_alternative<data::HeaderNameValueData>(action.data))
        {
          const auto& [name, value]
            = std::get<data::HeaderNameValueData>(action.data);
          request_adapter.AddHeader(name, value);
          return {};
        }
      return Unexpected(Error { Errc::InvalidAction,
                                "add header requires HeaderNameValueData" });
    }

  if(action.op == Op::Set)
    {
      if(std::holds_alternative<data::HeaderNameValueData>(action.data))
        {
          const auto& [name, value]
            = std::get<data::HeaderNameValueData>(action.data);
          request_adapter.SetHeader(name, value);
          return {};
        }
      return Unexpected(Error { Errc::InvalidAction,
                                "set header requires HeaderNameValueData" });
    }

  if(action.op == Op::Replace)
    {
      if(std::holds_alternative<data::HeaderNameValueData>(action.data))
        {
          const auto& [name, value]
            = std::get<data::HeaderNameValueData>(action.data);
          request_adapter.ReplaceHeader(name, value);
          return {};
        }
      return Unexpected(Error {
        Errc::InvalidAction, "replace header requires HeaderNameValueData" });
    }

  if(action.op == Op::Remove)
    {
      if(std::holds_alternative<data::HeaderNameData>(action.data))
        {
          const auto& [name] = std::get<data::HeaderNameData>(action.data);
          request_adapter.RemoveHeader(name);
          return {};
        }
      return Unexpected(Error { Errc::InvalidAction,
                                "remove header requires HeaderNameData" });
    }

  return Unexpected(
    Error { Errc::UnsupportedAction, "unsupported operation" });
}

proxypp::Result<void>
proxypp::rule::http::ExecuteResponseAction(const Action& action,
                                           ResponseAdapter& response_adapter)
{
  if(action.target != Target::Header)
    {
      return Unexpected(
        Error { Errc::UnsupportedAction, "unsupported target" });
    }

  if(std::holds_alternative<data::HeaderNameData>(action.data))
    {
      const auto& data = std::get<data::HeaderNameData>(action.data);
      if(kUnsupportedHeaders.contains(
           boost::algorithm::to_lower_copy(data.name)))
        {
          return Unexpected(
            Error { Errc::InvalidAction,
                    std::format("unsupported header {}", data.name) });
        }
    }

  if(std::holds_alternative<data::HeaderNameValueData>(action.data))
    {
      const auto& data = std::get<data::HeaderNameValueData>(action.data);
      if(kUnsupportedHeaders.contains(
           boost::algorithm::to_lower_copy(data.name)))
        {
          return Unexpected(
            Error { Errc::InvalidAction,
                    std::format("unsupported header {}", data.name) });
        }
    }

  if(action.op == Op::Add)
    {
      if(std::holds_alternative<data::HeaderNameValueData>(action.data))
        {
          const auto& [name, value]
            = std::get<data::HeaderNameValueData>(action.data);
          response_adapter.AddHeader(name, value);
          return {};
        }
      return Unexpected(Error { Errc::InvalidAction,
                                "add header requires HeaderNameValueData" });
    }

  if(action.op == Op::Set)
    {
      if(std::holds_alternative<data::HeaderNameValueData>(action.data))
        {
          const auto& [name, value]
            = std::get<data::HeaderNameValueData>(action.data);
          response_adapter.SetHeader(name, value);
          return {};
        }
      return Unexpected(Error { Errc::InvalidAction,
                                "set header requires HeaderNameValueData" });
    }

  if(action.op == Op::Replace)
    {
      if(std::holds_alternative<data::HeaderNameValueData>(action.data))
        {
          const auto& [name, value]
            = std::get<data::HeaderNameValueData>(action.data);
          response_adapter.ReplaceHeader(name, value);
          return {};
        }
      return Unexpected(Error {
        Errc::InvalidAction, "replace header requires HeaderNameValueData" });
    }

  if(action.op == Op::Remove)
    {
      if(std::holds_alternative<data::HeaderNameData>(action.data))
        {
          const auto& [name] = std::get<data::HeaderNameData>(action.data);
          response_adapter.RemoveHeader(name);
          return {};
        }
      return Unexpected(Error { Errc::InvalidAction,
                                "remove header requires HeaderNameData" });
    }

  return Unexpected(
    Error { Errc::UnsupportedAction, "unsupported operation" });
}