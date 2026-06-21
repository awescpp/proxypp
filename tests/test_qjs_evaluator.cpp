/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#define BOOST_TEST_MODULE test_qjs_evaluator

#include "qjs_fixture.h"
#include <boost/test/unit_test.hpp>
#include <proxypp/script/qjs.h>
#include <string>
#include <string_view>
#include <utility>

namespace proxypp::script::qjs::test
{
  BOOST_FIXTURE_TEST_SUITE(eval_success_tests, QjsContextFixture)

  BOOST_AUTO_TEST_CASE(eval_int32_expression_should_return_expected_number)
  {
    auto value = qjs::Evaluator::Eval(context, "1 + 2 + 3");
    BOOST_REQUIRE(value.has_value());
    BOOST_TEST(value->IsNumber());

    auto number = value->ToInt32();
    BOOST_REQUIRE(number.has_value());
    BOOST_TEST(*number == 6);
  }

  BOOST_AUTO_TEST_CASE(eval_bool_expression_should_return_expected_bool)
  {
    auto value = qjs::Evaluator::Eval(context, "1 < 2 && 3 > 2");
    BOOST_REQUIRE(value.has_value());
    BOOST_TEST(value->IsBool());

    auto boolean = value->ToBool();
    BOOST_REQUIRE(boolean.has_value());
    BOOST_TEST(*boolean == true);
  }

  BOOST_AUTO_TEST_CASE(eval_string_expression_should_return_expected_string)
  {
    auto value
      = qjs::Evaluator::Eval(context, "'proxy' + '++' + ' is awesome' + '!'");
    BOOST_REQUIRE(value.has_value());
    BOOST_TEST(value->IsString());

    auto text = value->ToString();
    BOOST_REQUIRE(text.has_value());
    BOOST_TEST(*text == "proxy++ is awesome!");
  }

  BOOST_AUTO_TEST_CASE(eval_null_expression_should_return_null_value)
  {
    auto value = qjs::Evaluator::Eval(context, "null");
    BOOST_REQUIRE(value.has_value());
    BOOST_TEST(value->IsValid());
    BOOST_TEST(value->IsNull());
  }

  BOOST_AUTO_TEST_CASE(eval_undefined_expression_should_return_undefined_value)
  {
    auto value = qjs::Evaluator::Eval(context, "undefined");
    BOOST_REQUIRE(value.has_value());
    BOOST_TEST(value->IsValid());
    BOOST_TEST(value->IsUndefined());
  }

  BOOST_AUTO_TEST_CASE(eval_object_literal_should_return_object_value)
  {
    auto value
      = qjs::Evaluator::Eval(context, "({name: 'proxy++', port: 8080})");
    BOOST_REQUIRE(value.has_value());
    BOOST_REQUIRE(value->IsValid());
    BOOST_TEST(value->IsObject());

    auto name = value->GetProperty("name");
    BOOST_REQUIRE(name.has_value());
    BOOST_REQUIRE(name->IsValid());
    BOOST_REQUIRE(name->IsString());
    auto name_text = name->ToString();
    BOOST_REQUIRE(name_text.has_value());
    BOOST_TEST(*name_text == "proxy++");

    auto port = value->GetProperty("port");
    BOOST_REQUIRE(port.has_value());
    BOOST_REQUIRE(port->IsValid());
    BOOST_REQUIRE(port->IsNumber());
    auto port_number = port->ToInt32();
    BOOST_REQUIRE(port_number.has_value());
    BOOST_TEST(*port_number == 8080);
  }

  BOOST_AUTO_TEST_CASE(
    eval_object_property_expression_should_return_expected_value)
  {
    auto value = qjs::Evaluator::Eval(
      context, "({request: {method: \"GET\"}}).request.method");
    BOOST_REQUIRE(value.has_value());
    BOOST_REQUIRE(value->IsValid());
    BOOST_REQUIRE(value->IsString());

    auto method = value->ToString();
    BOOST_REQUIRE(method.has_value());
    BOOST_TEST(*method == "GET");
  }

  BOOST_AUTO_TEST_SUITE_END()

  BOOST_FIXTURE_TEST_SUITE(eval_failure_tests, QjsContextFixture)

  BOOST_AUTO_TEST_CASE(eval_syntax_error_should_return_eval_failed_with_message)
  {
    auto value = qjs::Evaluator::Eval(context, "const =");
    RequireErrorCode(value, Errc::EvalFailed);
    RequireErrorMessageContains(value, "SyntaxError");
  }

  BOOST_AUTO_TEST_CASE(
    eval_reference_error_should_return_eval_failed_with_message)
  {
    auto value = qjs::Evaluator::Eval(context, "not_existed + 1");
    RequireErrorCode(value, Errc::EvalFailed);
    RequireErrorMessageContains(value, "ReferenceError");
  }

  BOOST_AUTO_TEST_CASE(eval_type_error_should_return_eval_failed_with_message)
  {
    auto value = qjs::Evaluator::Eval(context, "null.foo");
    RequireErrorCode(value, Errc::EvalFailed);
    RequireErrorMessageContains(value, "TypeError");
  }

  BOOST_AUTO_TEST_CASE(eval_throw_error_should_return_eval_failed_with_message)
  {
    auto value = qjs::Evaluator::Eval(context, "throw new Error('boom');");
    RequireErrorCode(value, Errc::EvalFailed);
    RequireErrorMessageContains(value, "boom");
  }

  BOOST_AUTO_TEST_CASE(eval_custom_file_name_should_appear_in_error_message)
  {
    auto value = qjs::Evaluator::Eval(
      context, "throw new Error('named failure')", "rules/test-rule.js");
    RequireErrorCode(value, Errc::EvalFailed);
    RequireErrorMessageContains(value, "named failure");
    RequireErrorMessageContains(value, "rules/test-rule.js");
  }

  BOOST_AUTO_TEST_SUITE_END()

  BOOST_FIXTURE_TEST_SUITE(invalid_context_tests, QjsContextFixture)

  BOOST_AUTO_TEST_CASE(eval_with_invalid_context_should_return_invalid_argument)
  {
    qjs::Context moved { std::move(context) };
    BOOST_REQUIRE(moved.NativeHandle() != nullptr);

    auto value = qjs::Evaluator::Eval(context, "1 + 1");
    RequireErrorCode(value, Errc::InvalidArgument);
  }

  BOOST_AUTO_TEST_SUITE_END()
}
