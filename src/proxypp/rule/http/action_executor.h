/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "model.h"
#include "proxypp/result.h"

namespace proxypp::rule::http
{
  class RequestAdapter;
  class ResponseAdapter;

  Result<void>
  ExecuteRequestAction(const Action& action, RequestAdapter& request_adapter);

  Result<void> ExecuteResponseAction(const Action& action,
                                     ResponseAdapter& response_adapter);

}