/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#define BOOST_TEST_MODULE test_rule_http_context_injector

#include "proxypp/http/adapter/beast_request_adapter.h"
#include "proxypp/http/adapter/beast_response_adapter.h"
#include "proxypp/rule/http/context_injector.h"
#include "proxypp/rule/match_context.h"
#include "proxypp/script/qjs.h"
#include "qjs_test_helper.h"
#include <boost/beast.hpp>
#include <boost/test/unit_test.hpp>
#include <memory>

namespace proxypp::rule::http::test
{
  namespace http_ = boost::beast::http;
  namespace qjs = proxypp::script::qjs;

  struct TestFixture
  {
    qjs::Runtime runtime;
    rule::MatchContext context;
    http_::request<http_::empty_body> request;
    http_::response<http_::empty_body> response;
    std::shared_ptr<rule::http::RequestAdapter> request_adapter;
    std::shared_ptr<rule::http::ResponseAdapter> response_adapter;

    static http_::request_header<> CreateRequestHeader()
    {
      http_::request_header<> header;
      header.method(http_::verb::get);
      header.target("/api/users");
      header.version(11);
      header.set(http_::field::host, "example.com");
      header.set(http_::field::user_agent, "proxypp-test");
      header.set("X-Test", "hello");
      return header;
    }

    static http_::response_header<> CreateResponseHeader()
    {
      http_::response_header<> header;
      header.result(http_::status::ok);
      header.reason("OK");
      header.version(11);
      header.set(http_::field::server, "proxypp-test-server");
      header.set("X-Response-Test", "world");
      return header;
    }

    Result<qjs::Value> Eval(std::string_view expr)
    {
      return qjs::Evaluator::Eval(context.ScriptContext(), expr);
    }

    TestFixture()
        : runtime(qjs::test::CreateRuntime()),
          context(qjs::test::CreateMatchContext(runtime)),
          request(CreateRequestHeader()), response(CreateResponseHeader()),
          request_adapter(
            std::make_shared<proxypp::http::adapter::BeastRequestAdapter>(
              request)),
          response_adapter(
            std::make_shared<proxypp::http::adapter::BeastResponseAdapter>(
              response))
    {}
  };

  BOOST_FIXTURE_TEST_SUITE(http_context_injector_tests, TestFixture)

  BOOST_AUTO_TEST_CASE(inject_request_context_should_inject_request_phase)
  {
    auto result = InjectRequestContext(context, *request_adapter);
    BOOST_REQUIRE(result.has_value());

    const auto phase = Eval("ctx.phase");
    BOOST_TEST(qjs::test::GetString(phase) == "request");
  }

  BOOST_AUTO_TEST_CASE(inject_request_context_should_inject_request_fields)
  {
    auto result = InjectRequestContext(context, *request_adapter);
    BOOST_REQUIRE(result.has_value());

    const auto request_method = Eval("ctx.request.method");
    BOOST_TEST(qjs::test::GetString(request_method) == "GET");

    const auto request_target = Eval("ctx.request.target");
    BOOST_TEST(qjs::test::GetString(request_target) == "/api/users");

    const auto request_version = Eval("ctx.request.version");
    BOOST_TEST(qjs::test::GetInt32(request_version) == 11);
  }

  BOOST_AUTO_TEST_CASE(
    inject_request_context_should_inject_lower_case_request_headers)
  {
    auto result = InjectRequestContext(context, *request_adapter);
    BOOST_REQUIRE(result.has_value());

    const auto user_agent = Eval("ctx.request.headers['user-agent']");
    BOOST_TEST(qjs::test::GetString(user_agent) == "proxypp-test");

    const auto x_test = Eval("ctx.request.headers['x-test']");
    BOOST_TEST(qjs::test::GetString(x_test) == "hello");
  }

  BOOST_AUTO_TEST_CASE(
    inject_request_context_should_inject_updated_request_header)
  {
    BOOST_REQUIRE(request.contains("x-test"));
    BOOST_REQUIRE(request["x-test"] == "hello");

    request_adapter->SetHeader("X-TEST", "world");

    auto result = InjectRequestContext(context, *request_adapter);
    BOOST_REQUIRE(result.has_value());

    const auto x_test = Eval("ctx.request.headers['x-test']");
    BOOST_TEST(qjs::test::GetString(x_test) == "world");
  }

  BOOST_AUTO_TEST_CASE(inject_request_context_should_not_inject_response_object)
  {
    auto result = InjectRequestContext(context, *request_adapter);
    BOOST_REQUIRE(result.has_value());

    const auto response_type = Eval("typeof ctx.response");
    BOOST_TEST(qjs::test::GetString(response_type) == "undefined");
  }

  // ----------------------------------------------------

  BOOST_AUTO_TEST_CASE(inject_response_context_should_inject_response_phase)
  {
    auto result
      = InjectResponseContext(context, *request_adapter, *response_adapter);
    BOOST_REQUIRE(result.has_value());

    const auto phase = Eval("ctx.phase");
    BOOST_TEST(qjs::test::GetString(phase) == "response");
  }

  BOOST_AUTO_TEST_CASE(
    inject_response_context_should_inject_request_and_response_fields)
  {
    auto result
      = InjectResponseContext(context, *request_adapter, *response_adapter);
    BOOST_REQUIRE(result.has_value());

    const auto request_method = Eval("ctx.request.method");
    BOOST_TEST(qjs::test::GetString(request_method) == "GET");

    const auto request_target = Eval("ctx.request.target");
    BOOST_TEST(qjs::test::GetString(request_target) == "/api/users");

    const auto request_version = Eval("ctx.request.version");
    BOOST_TEST(qjs::test::GetInt32(request_version) == 11);

    const auto host = Eval("ctx.request.headers['host']");
    BOOST_TEST(qjs::test::GetString(host) == "example.com");

    const auto response_status = Eval("ctx.response.status");
    BOOST_TEST(qjs::test::GetInt32(response_status) == 200);

    const auto response_reason = Eval("ctx.response.reason");
    BOOST_TEST(qjs::test::GetString(response_reason) == "OK");

    const auto response_version = Eval("ctx.response.version");
    BOOST_TEST(qjs::test::GetInt32(response_version) == 11);
  }

  BOOST_AUTO_TEST_CASE(
    inject_response_context_should_inject_lowercase_response_headers)
  {
    auto result
      = InjectResponseContext(context, *request_adapter, *response_adapter);
    BOOST_REQUIRE(result.has_value());

    const auto server = Eval("ctx.response.headers['server']");
    BOOST_TEST(qjs::test::GetString(server) == "proxypp-test-server");

    const auto x_response_test
      = Eval("ctx.response.headers['x-response-test']");
    BOOST_TEST(qjs::test::GetString(x_response_test) == "world");
  }

  BOOST_AUTO_TEST_SUITE_END()

}
