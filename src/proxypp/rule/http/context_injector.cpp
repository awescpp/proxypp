/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "proxypp/rule/http/context_injector.h"
proxypp::Result<void> proxypp::rule::http::InjectRequestContext(
  MatchContext& context, const RequestAdapter& request_adapter)
{
  return {};
}

proxypp::Result<void> proxypp::rule::http::InjectResponseContext(
  MatchContext& context, const RequestAdapter& request_adapter,
  const ResponseAdapter& response_adapter)
{
  return {};
}