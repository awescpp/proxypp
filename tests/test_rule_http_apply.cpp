/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#define BOOST_TEST_MODULE test_rule_http_apply

#include "proxypp/http/adapter/beast_request_adapter.h"
#include "proxypp/http/adapter/beast_response_adapter.h"
#include "proxypp/rule/error.h"
#include "proxypp/rule/http/apply.h"
#include "proxypp/rule/http/request_adapter.h"
#include "proxypp/rule/http/response_adapter.h"
#include <boost/beast.hpp>
#include <boost/test/unit_test.hpp>

namespace proxypp::rule::http::test
{
  namespace http_ = boost::beast::http;

  constexpr auto kTestHeaderName = "x-proxypp-test";
  constexpr auto kTestHeaderValue = "value";
  constexpr auto kAnotherTestHeaderValue = "another value";

  constexpr auto kUserAgentHeaderName = "user-agent";
  constexpr auto kUserAgentHeaderValue = "Boost.Beast";

  auto CreateAdapter(http_::request<http_::empty_body>& request)
  {
    return std::make_unique<proxypp::http::adapter::BeastRequestAdapter>(
      request);
  }

  auto CreateAdapter(http_::response<http_::empty_body>& response)
  {
    return std::make_unique<proxypp::http::adapter::BeastResponseAdapter>(
      response);
  }

  RuleEngine CreateEngine()
  {
    auto engine = RuleEngine::Create();
    BOOST_REQUIRE(engine.has_value());
    return std::move(*engine);
  }

  BOOST_AUTO_TEST_SUITE(apply_request_tests)

  BOOST_AUTO_TEST_CASE(apply_request_should_do_nothing_when_rules_is_not_set)
  {
    auto engine = CreateEngine();
    http_::request<http_::empty_body> request;
    auto adapter = CreateAdapter(request);
    const rule::http::Config http_config {};
    const auto result = ApplyRequest(engine, http_config, *adapter);
    BOOST_TEST(result.has_value());
  }

  BOOST_AUTO_TEST_CASE(
    apply_request_should_execute_enabled_request_phase_rule_without_match)
  {
    auto engine = CreateEngine();

    http_::request<http_::empty_body> request;
    auto adapter = CreateAdapter(request);

    const http::Config http_config {
      .rules
      = std::vector<http::Rule> { Rule {
                                    .name = "add a test header",
                                    .enabled = true,
                                    .phase = Phase::Request,
                                    .match = std::nullopt,
                                    .actions
                                    = std::vector<http::Action> { Action {
                                      .op = Op::Add,
                                      .target = Target::Header,
                                      .data
                                      = http::data::HeaderNameValueData { .name
                                                                          = kTestHeaderName,
                                                                          .value = kTestHeaderValue } } } },
                                  Rule { .name = "set user-agent",
                                         .enabled = true,
                                         .phase = Phase::Request,
                                         .match = std::nullopt,
                                         .actions = std::
                                           vector<
                                             http::Action> { Action { .op
                                                                      = Op::Set,
                                                                      .target
                                                                      = Target::Header,
                                                                      .data
                                                                      = http::data::HeaderNameValueData { .name
                                                                                                          = kUserAgentHeaderName,
                                                                                                          .value = kUserAgentHeaderValue } } } } }
    };

    const auto result = ApplyRequest(engine, http_config, *adapter);
    BOOST_REQUIRE(result.has_value());
    BOOST_TEST(request.count(kTestHeaderName) == 1);
    BOOST_TEST(request[kTestHeaderName] == kTestHeaderValue);
    BOOST_TEST(request.count(kUserAgentHeaderName) == 1);
    BOOST_TEST(request[kUserAgentHeaderName] == kUserAgentHeaderValue);
  }

  BOOST_AUTO_TEST_CASE(apply_request_should_ignore_disabled_rule)
  {
    auto engine = CreateEngine();

    http_::request<http_::empty_body> request;
    request.set(kUserAgentHeaderName, kUserAgentHeaderValue);
    BOOST_REQUIRE(request.count(kUserAgentHeaderName) == 1);

    auto adapter = CreateAdapter(request);

    const http::Config
      http_config { .rules = std::vector<http::Rule> {
                      Rule { .name = "remove user-agent",
                             .enabled = false,
                             .phase = Phase::Request,
                             .match = std::nullopt,
                             .actions = std::vector<http::Action> { Action {
                               .op = Op::Remove,
                               .target = Target::Header,
                               .data
                               = http::data::HeaderNameData { .name
                                                              = kUserAgentHeaderName } } } },
                    } };

    const auto result = ApplyRequest(engine, http_config, *adapter);
    BOOST_REQUIRE(result.has_value());
    BOOST_TEST(request.count(kUserAgentHeaderName) == 1);
    BOOST_TEST(request[kUserAgentHeaderName] == kUserAgentHeaderValue);
  }

  BOOST_AUTO_TEST_CASE(apply_request_should_ignore_response_phase_rule)
  {
    auto engine = CreateEngine();

    http_::request<http_::empty_body> request;
    request.set(kUserAgentHeaderName, kUserAgentHeaderValue);
    BOOST_REQUIRE(request.count(kUserAgentHeaderName) == 1);

    auto adapter = CreateAdapter(request);

    const http::Config
      http_config { .rules = std::vector<http::Rule> {
                      Rule { .name = "remove user-agent",
                             .enabled = true,
                             .phase = Phase::Response,
                             .match = std::nullopt,
                             .actions = std::vector<http::Action> { Action {
                               .op = Op::Remove,
                               .target = Target::Header,
                               .data
                               = http::data::HeaderNameData { .name
                                                              = kUserAgentHeaderName } } } },
                    } };

    const auto result = ApplyRequest(engine, http_config, *adapter);
    BOOST_REQUIRE(result.has_value());
    BOOST_TEST(request.count(kUserAgentHeaderName) == 1);
    BOOST_TEST(request[kUserAgentHeaderName] == kUserAgentHeaderValue);
  }

  BOOST_AUTO_TEST_CASE(apply_request_should_do_nothing_when_rules_is_empty)
  {
    auto engine = CreateEngine();

    http_::request<http_::empty_body> request;
    auto adapter = CreateAdapter(request);

    const http::Config http_config { .rules = std::vector<http::Rule> {} };

    const auto result = ApplyRequest(engine, http_config, *adapter);
    BOOST_TEST(result.has_value());
  }

  BOOST_AUTO_TEST_CASE(
    apply_request_should_return_error_when_action_execution_failed)
  {
    auto engine = CreateEngine();

    http_::request<http_::empty_body> request;
    auto adapter = CreateAdapter(request);

    const http::Config http_config {
      .rules = std::vector<http::Rule> { Rule {
        .name = "set content-length",
        .enabled = true,
        .phase = Phase::Request,
        .match = std::nullopt,
        .actions = std::vector<http::Action> { Action {
          .op = Op::Set,
          .target = Target::Header,
          // Content-Length is a unsupported header
          .data = http::data::HeaderNameValueData { .name = "Content-Length",
                                                    .value = "100" } } } } }
    };

    const auto result = ApplyRequest(engine, http_config, *adapter);
    BOOST_REQUIRE(!result.has_value());
    BOOST_TEST(result.error().code() == Errc::InvalidAction);
  }

  BOOST_AUTO_TEST_SUITE_END()

  // -------------------------apply_response_tests------------------------------

  BOOST_AUTO_TEST_SUITE(apply_response_tests)

  BOOST_AUTO_TEST_CASE(apply_response_should_do_nothing_when_rules_is_not_set)
  {
    auto engine = CreateEngine();

    http_::request<http_::empty_body> request;
    auto request_adapter = CreateAdapter(request);
    http_::response<http_::empty_body> response;
    auto response_adapter = CreateAdapter(response);

    const rule::http::Config http_config {};

    const auto result = ApplyResponse(engine, http_config, *request_adapter,
                                      *response_adapter);
    BOOST_TEST(result.has_value());
  }

  BOOST_AUTO_TEST_CASE(
    apply_response_should_execute_enabled_response_phase_rule_without_match)
  {
    auto engine = CreateEngine();

    http_::request<http_::empty_body> request;
    auto request_adapter = CreateAdapter(request);
    http_::response<http_::empty_body> response;
    auto response_adapter = CreateAdapter(response);

    response.set(kTestHeaderName, kTestHeaderValue);
    BOOST_REQUIRE(response.count(kTestHeaderName) == 1);

    const http::Config http_config {
      .rules = std::vector<http::Rule> { Rule {
        .name = "remove test header",
        .enabled = true,
        .phase = Phase::Response,
        .match = std::nullopt,
        .actions = std::vector<http::Action> { Action {
          .op = Op::Remove,
          .target = Target::Header,
          .data = http::data::HeaderNameData { .name = kTestHeaderName } } } } }
    };

    const auto result = ApplyResponse(engine, http_config, *request_adapter,
                                      *response_adapter);
    BOOST_REQUIRE(result.has_value());
    BOOST_TEST(response.count(kTestHeaderName) == 0);
  }

  BOOST_AUTO_TEST_CASE(apply_response_should_ignore_disabled_rule)
  {
    auto engine = CreateEngine();

    http_::request<http_::empty_body> request;
    auto request_adapter = CreateAdapter(request);
    http_::response<http_::empty_body> response;
    auto response_adapter = CreateAdapter(response);

    response.set(kTestHeaderName, kTestHeaderValue);
    BOOST_REQUIRE(response.count(kTestHeaderName) == 1);

    const http::Config http_config {
      .rules = std::vector<http::Rule> { Rule {
        .name = "remove test header",
        .enabled = false,
        .phase = Phase::Response,
        .match = std::nullopt,
        .actions = std::vector<http::Action> { Action {
          .op = Op::Remove,
          .target = Target::Header,
          .data = http::data::HeaderNameData { .name = kTestHeaderName } } } } }
    };

    const auto result = ApplyResponse(engine, http_config, *request_adapter,
                                      *response_adapter);
    BOOST_REQUIRE(result.has_value());
    BOOST_TEST(response.count(kTestHeaderName) == 1);
    BOOST_TEST(response[kTestHeaderName] == kTestHeaderValue);
  }

  BOOST_AUTO_TEST_CASE(apply_response_should_ignore_request_phase_rule)
  {
    auto engine = CreateEngine();

    http_::request<http_::empty_body> request;
    auto request_adapter = CreateAdapter(request);
    http_::response<http_::empty_body> response;
    auto response_adapter = CreateAdapter(response);

    response.set(kTestHeaderName, kTestHeaderValue);
    BOOST_REQUIRE(response.count(kTestHeaderName) == 1);

    const http::Config http_config {
      .rules = std::vector<http::Rule> { Rule {
        .name = "remove test header",
        .enabled = true,
        .phase = Phase::Request,
        .match = std::nullopt,
        .actions = std::vector<http::Action> { Action {
          .op = Op::Remove,
          .target = Target::Header,
          .data = http::data::HeaderNameData { .name = kTestHeaderName } } } } }
    };

    const auto result = ApplyResponse(engine, http_config, *request_adapter,
                                      *response_adapter);
    BOOST_REQUIRE(result.has_value());
    BOOST_TEST(response.count(kTestHeaderName) == 1);
    BOOST_TEST(response[kTestHeaderName] == kTestHeaderValue);
  }

  BOOST_AUTO_TEST_SUITE_END()

}