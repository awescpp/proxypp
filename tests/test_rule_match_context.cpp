/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#define BOOST_TEST_MODULE test_rule_match_context

#include "proxypp/rule/error.h"
#include "proxypp/rule/match_context.h"
#include "proxypp/script/qjs.h"
#include <boost/test/unit_test.hpp>

namespace proxypp::rule::test
{
  namespace qjs = proxypp::script::qjs;

  struct TestFixture
  {
    qjs::Runtime runtime;
    rule::MatchContext context;

    static qjs::Runtime CreateRuntime()
    {
      auto runtime = qjs::Runtime::Create();
      BOOST_REQUIRE(runtime.has_value());
      return std::move(*runtime);
    }

    static rule::MatchContext CreateMatchContext(qjs::Runtime& runtime)
    {
      auto context = qjs::Context::Create(runtime);
      BOOST_REQUIRE(context.has_value());
      auto result = rule::MatchContext::Create(std::move(*context));
      BOOST_REQUIRE(result.has_value());
      return std::move(*result);
    }

    TestFixture()
        : runtime(CreateRuntime()), context(CreateMatchContext(runtime))
    {}
  };

  BOOST_FIXTURE_TEST_SUITE(match_context_tests, TestFixture)

  BOOST_AUTO_TEST_CASE(add_global_value_should_expose_string_to_script)
  {
    auto value = qjs::Value::String(context.ScriptContext(), "proxy++");
    BOOST_REQUIRE(value.has_value());
    const auto add_result
      = context.AddGlobalValue("a_string", std::move(*value));
    BOOST_REQUIRE(add_result.has_value());

    auto result = qjs::Evaluator::Eval(context.ScriptContext(), "a_string");
    BOOST_REQUIRE(result.has_value());
    BOOST_REQUIRE(result->IsString());

    auto text = result->ToString();
    BOOST_REQUIRE(text.has_value());
    BOOST_TEST(*text == "proxy++");
  }

  BOOST_AUTO_TEST_CASE(add_global_value_should_expose_bool_to_script)
  {
    auto value = qjs::Value::Bool(context.ScriptContext(), true);
    BOOST_REQUIRE(value.has_value());
    const auto add_result
      = context.AddGlobalValue("a_bool", std::move(*value));
    BOOST_REQUIRE(add_result.has_value());

    auto result = qjs::Evaluator::Eval(context.ScriptContext(), "a_bool");
    BOOST_REQUIRE(result.has_value());
    BOOST_REQUIRE(result->IsBool());

    auto bool_value = result->ToBool();
    BOOST_REQUIRE(bool_value.has_value());
    BOOST_TEST(*bool_value == true);
  }

  BOOST_AUTO_TEST_CASE(add_global_value_should_expose_object_to_script)
  {
    auto request = qjs::Value::Object(context.ScriptContext());
    BOOST_REQUIRE(request.has_value());

    auto method = qjs::Value::String(context.ScriptContext(), "GET");
    BOOST_REQUIRE(method.has_value());

    auto set_result = request->SetProperty("method", std::move(*method));
    BOOST_REQUIRE(set_result.has_value());

    auto add_result = context.AddObject("request", std::move(*request));
    BOOST_REQUIRE(add_result.has_value());

    auto result
      = qjs::Evaluator::Eval(context.ScriptContext(), "request.method");
    BOOST_REQUIRE(result.has_value());

    auto text = result->ToString();
    BOOST_REQUIRE(text.has_value());
    BOOST_TEST(*text == "GET");
  }

  BOOST_AUTO_TEST_CASE(add_global_value_should_expose_null_to_script)
  {
    auto value = qjs::Value::Null(context.ScriptContext());
    BOOST_REQUIRE(value.has_value());

    auto add_result = context.AddGlobalValue("a_null", std::move(*value));
    BOOST_REQUIRE(add_result.has_value());

    const auto result
      = qjs::Evaluator::Eval(context.ScriptContext(), "a_null === null");
    BOOST_REQUIRE(result.has_value());
    BOOST_REQUIRE(result->IsBool());

    auto matched = result->ToBool();
    BOOST_REQUIRE(matched.has_value());
    BOOST_TEST(*matched == true);
  }

  BOOST_AUTO_TEST_CASE(add_object_should_reject_non_object_value)
  {
    auto null_value = qjs::Value::Null(context.ScriptContext());
    BOOST_REQUIRE(null_value.has_value());

    auto add_result = context.AddObject("an_obj", std::move(*null_value));
    BOOST_REQUIRE(!add_result.has_value());
    BOOST_TEST(add_result.error().code == Errc::ContextInjectionFailed);
  }

  BOOST_AUTO_TEST_CASE(
    add_global_value_should_overwrite_existing_global_property)
  {
    constexpr auto kPropertyName = "prop";

    auto value = qjs::Value::String(context.ScriptContext(), "value");
    BOOST_REQUIRE(value.has_value());

    auto add_result = context.AddGlobalValue(kPropertyName, std::move(*value));
    BOOST_REQUIRE(add_result.has_value());

    auto another_value = qjs::Value::Int32(context.ScriptContext(), 42);
    BOOST_REQUIRE(another_value.has_value());

    add_result
      = context.AddGlobalValue(kPropertyName, std::move(*another_value));
    BOOST_REQUIRE(add_result.has_value());

    auto got = qjs::Evaluator::Eval(context.ScriptContext(), kPropertyName);
    BOOST_REQUIRE(got.has_value());
    BOOST_REQUIRE(got->IsNumber());

    auto int_val = got->ToInt32();
    BOOST_REQUIRE(int_val.has_value());

    BOOST_TEST(*int_val == 42);
  }

  BOOST_AUTO_TEST_CASE(
    add_global_value_should_reject_value_from_different_context)
  {
    auto another_context = qjs::Context::Create(runtime);
    BOOST_REQUIRE(another_context.has_value());

    auto value = qjs::Value::String(*another_context, "proxy++");
    BOOST_REQUIRE(value.has_value());

    auto add_result = context.AddGlobalValue("prop", std::move(*value));
    BOOST_REQUIRE(!add_result.has_value());

    BOOST_TEST(add_result.error().code == Errc::ContextInjectionFailed);
  }

  BOOST_AUTO_TEST_SUITE_END()
}