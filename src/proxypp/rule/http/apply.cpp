/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "proxypp/rule/http/apply.h"
#include "proxypp/rule/http/action_executor.h"
#include "proxypp/rule/http/context_injector.h"
#include "proxypp/rule/match_context.h"

proxypp::Result<void>
proxypp::rule::http::ApplyRequest(RuleEngine& engine,
                                  const Config& http_config,
                                  RequestAdapter& request_adapter)
{
  if(!http_config.rules)
    {
      return {};
    }
  return engine.ApplyRules(
    http_config.rules.value(),
    [](const Rule& rule) {
      return rule.enabled && rule.phase == Phase::Request;
    },
    [&request_adapter](MatchContext& context) {
      return InjectRequestContext(context, request_adapter);
    },
    [](const Rule& rule) -> const std::optional<Match>& { return rule.match; },
    [](const Rule& rule) -> const std::vector<Action>& {
      return rule.actions;
    },
    [&request_adapter](const Action& action) {
      return ExecuteRequestAction(action, request_adapter);
    });
}

proxypp::Result<void>
proxypp::rule::http::ApplyResponse(RuleEngine& engine,
                                   const Config& http_config,
                                   RequestAdapter& request_adapter,
                                   ResponseAdapter& response_adapter)
{
  if(!http_config.rules)
    {
      return {};
    }
  return engine.ApplyRules(
    http_config.rules.value(),
    [](const Rule& rule) {
      return rule.enabled && rule.phase == Phase::Response;
    },
    [&request_adapter, &response_adapter](MatchContext& context) {
      return InjectResponseContext(context, request_adapter, response_adapter);
    },
    [](const Rule& rule) -> const std::optional<Match>& { return rule.match; },
    [](const Rule& rule) -> const std::vector<Action>& {
      return rule.actions;
    },
    [&response_adapter](const Action& action) {
      return ExecuteResponseAction(action, response_adapter);
    });
}