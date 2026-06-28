/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "action_executor.h"
#include "model.h"
#include "proxypp/result.h"
#include "proxypp/rule/rule_engine.h"

namespace proxypp::rule::http
{
  Result<void> ApplyRequest(RuleEngine& engine, const Config& http_config,
                            RequestAdapter& request_adapter);

  Result<void> ApplyResponse(RuleEngine& engine, const Config& http_config,
                             RequestAdapter& request_adapter,
                             ResponseAdapter& response_adapter);
}