/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#define BOOST_TEST_MODULE test_rule_http_action_executor

#include "proxypp/http/adapter/beast_request_adapter.h"
#include "proxypp/http/adapter/beast_response_adapter.h"
#include "proxypp/rule/error.h"
#include "proxypp/rule/http/action_executor.h"
#include "proxypp/rule/http/model.h"
#include <boost/beast.hpp>
#include <boost/test/unit_test.hpp>
#include <type_traits>

namespace proxypp::rule::http::test
{
  namespace http_ = boost::beast::http;
  namespace http = proxypp::http;

  BOOST_AUTO_TEST_SUITE(request_and_response_action_executor_shared_tests)

  // region utility functions

  auto ExecuteAction(const Action& action,
                     http::adapter::BeastRequestAdapter& adapter)
  {
    return ExecuteRequestAction(action, adapter);
  }

  auto ExecuteAction(const Action& action,
                     http::adapter::BeastResponseAdapter& adapter)
  {
    return ExecuteResponseAction(action, adapter);
  }

  auto GetAdapter(http_::request<http_::empty_body>& request)
  {
    // http_::request<http_::empty_body> is also an http_::request_header<>
    return std::make_unique<http::adapter::BeastRequestAdapter>(request);
  }

  auto GetAdapter(http_::response<http_::empty_body>& response)
  {
    return std::make_unique<http::adapter::BeastResponseAdapter>(response);
  }

  template <typename Message>
    requires(std::is_same_v<Message, http_::request<http_::empty_body>>
             || std::is_same_v<Message, http_::response<http_::empty_body>>)
  auto PerformActionOnMessage(const Action& action, Message& message)
  {
    const auto adapter = GetAdapter(message);
    return ExecuteAction(action, *adapter);
  }

  // endregion

  BOOST_AUTO_TEST_CASE(add_supported_header_should_add_header)
  {
    const Action action { .op = Op::Add,
                          .target = Target::Header,
                          .data = data::HeaderNameValueData { "x-proxypp-test",
                                                              "ok" } };

    auto TestMessage = [&](auto& message) {
      auto result = PerformActionOnMessage(action, message);
      BOOST_REQUIRE(result.has_value());
      BOOST_TEST(message["x-proxypp-test"] == "ok");
    };

    http_::request<http_::empty_body> request;
    TestMessage(request);

    http_::response<http_::empty_body> response;
    TestMessage(response);
  }

  BOOST_AUTO_TEST_CASE(invalid_data_should_return_invalid_action_error)
  {
    const Action action { .op = Op::Remove,
                          .target = Target::Header,
                          .data = data::HeaderNameValueData { "x-proxypp-test",
                                                              "ok" } };

    auto TestMessage = [&](auto& message) {
      auto result = PerformActionOnMessage(action, message);
      BOOST_REQUIRE(!result.has_value());
      BOOST_TEST(result.error().code() == Errc::InvalidAction);
    };

    http_::request<http_::empty_body> request;
    TestMessage(request);

    http_::response<http_::empty_body> response;
    TestMessage(response);
  }

  BOOST_AUTO_TEST_CASE(invalid_op_should_return_invalid_action)
  {
    Action action { .op = Op::NotSet,
                    .target = Target::Header,
                    .data
                    = data::HeaderNameValueData { "x-proxypp-test", "ok" } };

    auto TestMessage = [&](auto& message) {
      auto result = PerformActionOnMessage(action, message);
      BOOST_REQUIRE(!result.has_value());
      BOOST_TEST(result.error().code() == Errc::InvalidAction);
    };

    http_::request<http_::empty_body> request;
    TestMessage(request);

    http_::response<http_::empty_body> response;
    TestMessage(response);
  }

  BOOST_AUTO_TEST_CASE(invalid_target_should_return_invalid_action)
  {
    Action action { .op = Op::Remove,
                    .target = Target::NotSet,
                    .data = data::HeaderNameData { "x-proxypp-test" } };

    auto TestMessage = [&](auto& message) {
      auto result = PerformActionOnMessage(action, message);
      BOOST_REQUIRE(!result.has_value());
      BOOST_TEST(result.error().code() == Errc::InvalidAction);
    };

    http_::request<http_::empty_body> request;
    TestMessage(request);

    http_::response<http_::empty_body> response;
    TestMessage(response);
  }

  BOOST_AUTO_TEST_CASE(
    given_forbidden_header_should_return_invalid_action_error)
  {
    const Action action { .op = Op::Add,
                          .target = Target::Header,
                          .data = data::HeaderNameValueData {
                            .name = "content-length", .value = "663" } };

    auto TestMessage = [&](auto& message) {
      auto result = PerformActionOnMessage(action, message);
      BOOST_REQUIRE(!result);
      BOOST_TEST(result.error().code() == Errc::InvalidAction);
    };

    http_::request<http_::empty_body> request;
    TestMessage(request);

    http_::response<http_::empty_body> response;
    TestMessage(response);
  }

  BOOST_AUTO_TEST_CASE(forbidden_header_should_case_insensitive)
  {
    const Action action { .op = Op::Add,
                          .target = Target::Header,
                          .data = data::HeaderNameValueData {
                            .name = "Content-Length", .value = "663" } };

    auto TestMessage = [&](auto& message) {
      auto result = PerformActionOnMessage(action, message);
      BOOST_REQUIRE(!result);
      BOOST_TEST(result.error().code() == Errc::InvalidAction);
    };

    http_::request<http_::empty_body> request;
    TestMessage(request);

    http_::response<http_::empty_body> response;
    TestMessage(response);
  }

  BOOST_AUTO_TEST_CASE(set_replace_remove_action_should_return_success)
  {
    const std::array actions = {
      Action { .op = Op::Set,
               .target = Target::Header,
               .data = data::HeaderNameValueData { .name = "x-proxypp-test",
                                                   .value = "ok" } },
      Action { .op = Op::Replace,
               .target = Target::Header,
               .data = data::HeaderNameValueData { .name = "x-proxypp-test",
                                                   .value = "ok" } },
      Action { .op = Op::Remove,
               .target = Target::Header,
               .data = data::HeaderNameData { .name = "x-proxypp-test" } }
    };

    auto TestMessage = [&](auto& message) {
      for(const auto& action : actions)
        {
          BOOST_TEST_CONTEXT("{} action should return success" << action.op)
          {
            const auto result = PerformActionOnMessage(action, message);
            BOOST_TEST(result.has_value());
          }
        }
    };

    http_::request<http_::empty_body> request;
    TestMessage(request);

    http_::response<http_::empty_body> response;
    TestMessage(response);
  }

  BOOST_AUTO_TEST_SUITE_END()

}