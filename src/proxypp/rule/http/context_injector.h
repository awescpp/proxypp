/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "proxypp/result.h"

namespace proxypp::rule
{
  class MatchContext;
}

namespace proxypp::rule::http
{
  class RequestAdapter;
  class ResponseAdapter;

  Result<void> InjectRequestContext(MatchContext& context,
                                    const RequestAdapter& request_adapter);

  Result<void> InjectResponseContext(MatchContext& context,
                                     const RequestAdapter& request_adapter,
                                     const ResponseAdapter& response_adapter);

}