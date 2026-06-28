/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "proxypp/rule/http/action_executor.h"
#include "proxypp/rule/error.h"
#include "proxypp/rule/op.h"
#include "response_adapter.h"

proxypp::Result<void>
proxypp::rule::http::ExecuteRequestAction(const Action& action,
                                          RequestAdapter& request_adapter)
{
  return {};
}

proxypp::Result<void>
proxypp::rule::http::ExecuteResponseAction(const Action& action,
                                           ResponseAdapter& response_adapter)
{
  if(action.target == Target::Header)
    {
      if(action.op == Op::Add)
        {
          const auto* data
            = std::get_if<data::HeaderNameValueData>(&action.data);
          if(data == nullptr)
            {
              Error error;
              error.code = Errc::InvalidAction;
              error.message = "add header action requires HeaderNameValueData";
              return Unexpected(error);
            }
          response_adapter.AddHeader(data->name, data->value);
          return {};
        }
    }

  return {};
}