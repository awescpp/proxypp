/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#define BOOST_TEST_MODULE test_qjs_value

#include "proxypp/script/qjs.h"
#include "qjs_fixture.h"
#include <boost/test/unit_test.hpp>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace proxypp::script::qjs::test
{
  BOOST_FIXTURE_TEST_SUITE(value_move_tests, QjsContextFixture)

  BOOST_AUTO_TEST_CASE(value_should_be_move_only)
  {
    BOOST_TEST(!std::is_copy_constructible_v<qjs::Value>);
    BOOST_TEST(!std::is_copy_assignable_v<qjs::Value>);
    BOOST_TEST(std::is_move_constructible_v<qjs::Value>);
    BOOST_TEST(std::is_move_assignable_v<qjs::Value>);
    BOOST_TEST(std::is_nothrow_move_constructible_v<qjs::Value>);
    BOOST_TEST(std::is_nothrow_move_assignable_v<qjs::Value>);
  }

  BOOST_AUTO_TEST_CASE(value_move_construct_should_transfer_value_ownership)
  {
    auto value = qjs::Value::Int32(context, 42);
    BOOST_REQUIRE(value.has_value());
    BOOST_REQUIRE(value->IsValid());

    qjs::Value moved { std::move(*value) };
    BOOST_TEST(!value->IsValid());
    BOOST_TEST(moved.IsValid());
    BOOST_TEST(moved.IsNumber());

    auto result = moved.ToInt32();
    BOOST_REQUIRE(result.has_value());
    BOOST_TEST(*result == 42);
  }

  BOOST_AUTO_TEST_CASE(
    value_move_assign_should_release_old_value_and_take_new_value)
  {
    auto lhs = qjs::Value::String(context, "old");
    BOOST_REQUIRE(lhs.has_value());
    BOOST_REQUIRE(lhs->IsValid());

    auto rhs = qjs::Value::String(context, "new");
    BOOST_REQUIRE(rhs.has_value());
    BOOST_REQUIRE(rhs->IsValid());

    *lhs = std::move(*rhs);

    BOOST_TEST(!rhs->IsValid());
    BOOST_TEST(lhs->IsValid());
    BOOST_TEST(lhs->IsString());
    auto result = lhs->ToString();
    BOOST_REQUIRE(result.has_value());
    BOOST_TEST(*result == "new");
  }

  BOOST_AUTO_TEST_CASE(value_self_move_assign_should_keep_value_valid)
  {
    auto value = qjs::Value::String(context, "proxy++");
    BOOST_REQUIRE(value.has_value());
    BOOST_REQUIRE(value->IsValid());

    *value = std::move(*value);
    BOOST_TEST(value->IsValid());

    auto result = value->ToString();
    BOOST_REQUIRE(result.has_value());
    BOOST_TEST(*result == "proxy++");
  }

  BOOST_AUTO_TEST_CASE(moved_from_value_should_be_invalid)
  {
    auto value = qjs::Value::String(context, "test");
    BOOST_REQUIRE(value.has_value());
    BOOST_REQUIRE(value->IsValid());

    auto moved = qjs::Value { std::move(*value) };
    BOOST_REQUIRE(moved.IsValid());
    BOOST_REQUIRE(moved.IsString());

    BOOST_TEST(!value->IsValid());
  }

  BOOST_AUTO_TEST_CASE(
    operations_on_moved_from_value_should_return_invalid_argument)
  {
    auto value = qjs::Value::String(context, "test");
    BOOST_REQUIRE(value.has_value());
    BOOST_REQUIRE(value->IsValid());

    auto moved = qjs::Value { std::move(*value) };
    BOOST_REQUIRE(moved.IsValid());
    BOOST_REQUIRE(moved.IsString());

    // moved from value should not valid anymore
    BOOST_TEST(!value->IsValid());
    auto result = value->ToString();
    BOOST_REQUIRE(!result.has_value());
    BOOST_TEST(result.error().code() == Errc::InvalidArgument);
  }

  BOOST_AUTO_TEST_CASE(
    value_move_assign_from_invalid_value_should_make_lhs_invalid)
  {
    auto lhs = qjs::Value::String(context, "lhs");
    BOOST_REQUIRE(lhs.has_value());
    BOOST_REQUIRE(lhs->IsValid());

    auto rhs = qjs::Value::String(context, "rhs");
    BOOST_REQUIRE(rhs.has_value());
    BOOST_REQUIRE(rhs->IsValid());

    qjs::Value moved { std::move(*rhs) };
    BOOST_REQUIRE(moved.IsValid());
    BOOST_TEST(!rhs->IsValid());

    *lhs = std::move(*rhs);
    BOOST_TEST(!lhs->IsValid());
  }

  BOOST_AUTO_TEST_CASE(
    value_move_assign_to_moved_from_value_should_take_new_value)
  {
    auto lhs = qjs::Value::String(context, "lhs");
    BOOST_REQUIRE(lhs.has_value());
    BOOST_REQUIRE(lhs->IsValid());

    // move lhs to moved, so here lhs is invalid
    auto moved = std::move(*lhs);
    BOOST_REQUIRE(moved.IsValid());
    BOOST_TEST(!lhs->IsValid());

    auto rhs = qjs::Value::String(context, "rhs");
    BOOST_REQUIRE(rhs.has_value());
    BOOST_REQUIRE(rhs->IsValid());

    // move rhs to an `invalid` lhs which makes lhs `valid` again
    *lhs = std::move(*rhs);
    BOOST_TEST(lhs->IsValid());
    BOOST_TEST(!rhs->IsValid());

    BOOST_REQUIRE(lhs->IsString());
    auto text = lhs->ToString();
    BOOST_REQUIRE(text.has_value());
    BOOST_TEST(*text == "rhs");
  }

  BOOST_AUTO_TEST_SUITE_END()

  BOOST_FIXTURE_TEST_SUITE(value_creation_tests, QjsContextFixture)

  BOOST_AUTO_TEST_CASE(
    context_global_object_should_be_valid_and_identified_as_object)
  {
    auto value = qjs::Value::GlobalObject(context);
    BOOST_REQUIRE(value.has_value());
    BOOST_REQUIRE(value->IsValid());
    BOOST_TEST(!value->IsUndefined());
    BOOST_TEST(!value->IsNull());
    BOOST_TEST(!value->IsBool());
    BOOST_TEST(!value->IsNumber());
    BOOST_TEST(!value->IsString());
    BOOST_TEST(value->IsObject());
  }

  BOOST_AUTO_TEST_CASE(
    global_object_set_property_should_be_visible_to_evaluator)
  {
    auto global = qjs::Value::GlobalObject(context);
    BOOST_REQUIRE(global.has_value());

    auto value = qjs::Value::String(context, "proxy++");
    BOOST_REQUIRE(value.has_value());

    const auto set_result
      = global->SetProperty("__proxypp_test_name", std::move(*value));
    BOOST_REQUIRE(set_result.has_value());

    auto result = qjs::Evaluator::Eval(context, "__proxypp_test_name");
    BOOST_REQUIRE(result.has_value());
    BOOST_REQUIRE(result->IsValid());

    auto text = result->ToString();
    BOOST_REQUIRE(text.has_value());
    BOOST_TEST(*text == "proxy++");
  }

  BOOST_AUTO_TEST_CASE(
    undefined_value_should_be_valid_and_identified_as_undefined)
  {
    auto value = qjs::Value::Undefined(context);
    BOOST_REQUIRE(value.has_value());
    BOOST_REQUIRE(value->IsValid());
    BOOST_TEST(value->IsUndefined());
    BOOST_TEST(!value->IsNull());
    BOOST_TEST(!value->IsBool());
    BOOST_TEST(!value->IsNumber());
    BOOST_TEST(!value->IsString());
    BOOST_TEST(!value->IsObject());
  }

  BOOST_AUTO_TEST_CASE(null_value_should_be_valid_and_identified_as_null)
  {
    auto value = qjs::Value::Null(context);
    BOOST_REQUIRE(value.has_value());
    BOOST_REQUIRE(value->IsValid());
    BOOST_TEST(value->IsNull());
    BOOST_TEST(!value->IsUndefined());
    BOOST_TEST(!value->IsBool());
    BOOST_TEST(!value->IsNumber());
    BOOST_TEST(!value->IsString());
    BOOST_TEST(!value->IsObject());
  }

  BOOST_AUTO_TEST_CASE(bool_true_value_should_convert_to_true)
  {
    auto value = qjs::Value::Bool(context, true);
    BOOST_REQUIRE(value.has_value());
    BOOST_REQUIRE(value->IsValid());
    BOOST_TEST(value->IsBool());
    BOOST_TEST(!value->IsNull());
    BOOST_TEST(!value->IsUndefined());
    BOOST_TEST(!value->IsNumber());
    BOOST_TEST(!value->IsString());
    BOOST_TEST(!value->IsObject());

    auto result = value->ToBool();
    BOOST_REQUIRE(result.has_value());
    BOOST_TEST(*result == true);
  }

  BOOST_AUTO_TEST_CASE(bool_false_value_should_convert_to_false)
  {
    auto value = qjs::Value::Bool(context, false);
    BOOST_REQUIRE(value.has_value());
    BOOST_REQUIRE(value->IsValid());
    BOOST_TEST(value->IsBool());
    BOOST_TEST(!value->IsNull());
    BOOST_TEST(!value->IsUndefined());
    BOOST_TEST(!value->IsNumber());
    BOOST_TEST(!value->IsString());
    BOOST_TEST(!value->IsObject());

    auto result = value->ToBool();
    BOOST_REQUIRE(result.has_value());
    BOOST_TEST(*result == false);
  }

  BOOST_AUTO_TEST_CASE(int32_value_should_convert_back_to_same_int32)
  {
    auto value = qjs::Value::Int32(context, 42);
    BOOST_REQUIRE(value.has_value());
    BOOST_REQUIRE(value->IsValid());

    BOOST_TEST(value->IsNumber());
    BOOST_TEST(!value->IsBool());
    BOOST_TEST(!value->IsNull());
    BOOST_TEST(!value->IsUndefined());
    BOOST_TEST(!value->IsString());
    BOOST_TEST(!value->IsObject());

    auto result = value->ToInt32();
    BOOST_REQUIRE(result.has_value());
    BOOST_TEST(*result == 42);
  }

  BOOST_AUTO_TEST_CASE(string_value_should_convert_back_to_same_string)
  {
    auto value = Value::String(context, "proxy++");
    BOOST_REQUIRE(value.has_value());
    BOOST_TEST(value->IsValid());
    BOOST_TEST(value->IsString());

    auto result = value->ToString();
    BOOST_REQUIRE(result.has_value());
    BOOST_TEST(*result == "proxy++");
  }

  BOOST_AUTO_TEST_CASE(object_value_should_be_valid_object)
  {
    auto value = Value::Object(context);
    BOOST_REQUIRE(value.has_value());
    BOOST_TEST(value->IsValid());
    BOOST_TEST(value->IsObject());
    BOOST_TEST(!value->IsArray());
  }

  BOOST_AUTO_TEST_SUITE_END()

  BOOST_FIXTURE_TEST_SUITE(string_view_creation_tests, QjsContextFixture)

  BOOST_AUTO_TEST_CASE(
    string_view_prefix_should_create_string_with_exact_length)
  {
    std::string text = "hello world";
    std::string_view s_view { text.data(), 5 };

    auto value = Value::String(context, s_view);
    BOOST_REQUIRE(value.has_value());

    auto result = value->ToString();
    BOOST_REQUIRE(result.has_value());
    BOOST_TEST(*result == "hello");
  }

  BOOST_AUTO_TEST_CASE(empty_string_view_should_create_empty_javascript_string)
  {
    auto value = qjs::Value::String(context, std::string_view {});
    BOOST_REQUIRE(value.has_value());
    BOOST_REQUIRE(value->IsString());
    auto result = value->ToString();
    BOOST_REQUIRE(result.has_value());
    BOOST_TEST(*result == "");
  }

  BOOST_AUTO_TEST_CASE(
    string_view_with_embedded_null_should_preserve_full_length)
  {
    std::string text { "abc\0def", 7 };
    std::string_view s_view { text.data(), text.size() };

    auto value = Value::String(context, s_view);
    BOOST_REQUIRE(value.has_value());
    BOOST_REQUIRE(value->IsString());

    auto result = value->ToString();
    BOOST_REQUIRE(result.has_value());
    BOOST_TEST(result->size() == text.size());
    BOOST_TEST(*result == text);
  }

  BOOST_AUTO_TEST_SUITE_END()

  BOOST_FIXTURE_TEST_SUITE(property_access_tests, QjsContextFixture)

  BOOST_AUTO_TEST_CASE(set_property_should_consume_input_value)
  {
    auto object = qjs::Value::Object(context);
    BOOST_REQUIRE(object.has_value());
    BOOST_REQUIRE(object->IsValid());

    auto value = qjs::Value::String(context, "test");
    BOOST_REQUIRE(value.has_value());
    BOOST_REQUIRE(value->IsValid());

    auto set_result = object->SetProperty("property", std::move(*value));
    BOOST_REQUIRE(set_result.has_value());

    // value is controlled by object now
    BOOST_TEST(!value->IsValid());

    auto got = object->GetProperty("property");
    BOOST_REQUIRE(got.has_value());
    BOOST_REQUIRE(got->IsValid());
    BOOST_REQUIRE(got->IsString());

    auto text = got->ToString();
    BOOST_REQUIRE(text.has_value());
    BOOST_TEST(*text == "test");
  }

  BOOST_AUTO_TEST_CASE(
    set_property_with_string_value_should_store_same_string_value)
  {
    auto obj = qjs::Value::Object(context);
    BOOST_REQUIRE(obj.has_value());
    BOOST_REQUIRE(obj->IsValid());

    auto s_val = qjs::Value::String(context, "test");
    BOOST_REQUIRE(s_val.has_value());
    BOOST_REQUIRE(s_val->IsValid());
    BOOST_REQUIRE(s_val->IsString());

    auto set_result = obj->SetProperty("message", std::move(*s_val));
    BOOST_REQUIRE(set_result.has_value());

    auto got = obj->GetProperty("message");
    BOOST_REQUIRE(got.has_value());
    BOOST_REQUIRE(got->IsString());
    auto text = got->ToString();
    BOOST_REQUIRE(text.has_value());
    BOOST_TEST(*text == "test");
  }

  BOOST_AUTO_TEST_CASE(
    set_property_with_int32_value_should_store_same_int32_value)
  {
    auto obj = qjs::Value::Object(context);
    BOOST_REQUIRE(obj.has_value());
    BOOST_REQUIRE(obj->IsValid());

    auto i_val = qjs::Value::Int32(context, 42);
    BOOST_REQUIRE(i_val.has_value());
    BOOST_REQUIRE(i_val->IsValid());
    BOOST_REQUIRE(i_val->IsNumber());

    auto set_result = obj->SetProperty("value", std::move(*i_val));
    BOOST_REQUIRE(set_result.has_value());

    auto got = obj->GetProperty("value");
    BOOST_REQUIRE(got.has_value());
    BOOST_TEST(got->IsNumber());
    auto number = got->ToInt32();
    BOOST_REQUIRE(number.has_value());
    BOOST_TEST(*number == 42);
  }

  BOOST_AUTO_TEST_CASE(
    set_property_with_true_bool_value_should_store_true_bool_value)
  {
    auto object = qjs::Value::Object(context);
    BOOST_REQUIRE(object.has_value());
    BOOST_REQUIRE(object->IsValid());

    auto value = qjs::Value::Bool(context, true);
    BOOST_REQUIRE(value.has_value());
    BOOST_REQUIRE(value->IsValid());
    BOOST_REQUIRE(value->IsBool());

    auto set_result = object->SetProperty("name", std::move(*value));
    BOOST_REQUIRE(set_result.has_value());

    auto got = object->GetProperty("name");
    BOOST_REQUIRE(got.has_value());
    BOOST_TEST(got->IsBool());

    auto got_value = got->ToBool();
    BOOST_REQUIRE(got_value.has_value());
    BOOST_TEST(*got_value == true);
  }

  BOOST_AUTO_TEST_CASE(
    set_property_with_false_bool_value_should_store_false_bool_value)
  {
    auto object = qjs::Value::Object(context);
    BOOST_REQUIRE(object.has_value());
    BOOST_REQUIRE(object->IsValid());
    BOOST_REQUIRE(object->IsObject());

    auto value = qjs::Value::Bool(context, false);
    BOOST_REQUIRE(value.has_value());
    BOOST_REQUIRE(value->IsValid());
    BOOST_REQUIRE(value->IsBool());

    auto set_result = object->SetProperty("name", std::move(*value));
    BOOST_REQUIRE(set_result.has_value());

    auto got = object->GetProperty("name");
    BOOST_REQUIRE(got.has_value());
    BOOST_REQUIRE(got->IsValid());
    BOOST_REQUIRE(got->IsBool());

    auto got_value = got->ToBool();
    BOOST_REQUIRE(got_value.has_value());
    BOOST_TEST(*got_value == false);
  }

  BOOST_AUTO_TEST_CASE(
    set_property_with_null_value_should_store_same_null_value)
  {
    auto obj = qjs::Value::Object(context);
    BOOST_REQUIRE(obj.has_value());
    BOOST_REQUIRE(obj->IsValid());

    auto null_val = qjs::Value::Null(context);
    BOOST_REQUIRE(null_val.has_value());
    BOOST_REQUIRE(null_val->IsValid());
    BOOST_REQUIRE(null_val->IsNull());

    auto set_result = obj->SetProperty("property", std::move(*null_val));
    BOOST_REQUIRE(set_result.has_value());

    auto got = obj->GetProperty("property");
    BOOST_REQUIRE(got.has_value());
    BOOST_TEST(got->IsNull());
  }

  BOOST_AUTO_TEST_CASE(
    set_property_with_undefined_value_should_store_same_undefined_value)
  {
    auto obj = qjs::Value::Object(context);
    BOOST_REQUIRE(obj.has_value());
    BOOST_REQUIRE(obj->IsValid());

    auto undefined_val = qjs::Value::Undefined(context);
    BOOST_REQUIRE(undefined_val.has_value());
    BOOST_REQUIRE(undefined_val->IsValid());
    BOOST_REQUIRE(undefined_val->IsUndefined());

    auto set_result = obj->SetProperty("property", std::move(*undefined_val));
    BOOST_REQUIRE(set_result.has_value());

    auto got = obj->GetProperty("property");
    BOOST_REQUIRE(got.has_value());
    BOOST_TEST(got->IsUndefined());
  }

  BOOST_AUTO_TEST_CASE(get_missing_property_should_return_undefined)
  {
    auto obj = qjs::Value::Object(context);
    BOOST_REQUIRE(obj.has_value());
    BOOST_REQUIRE(obj->IsValid());

    auto value = obj->GetProperty("unexisted_property");
    BOOST_REQUIRE(value.has_value());
    BOOST_REQUIRE(value->IsValid());
    BOOST_TEST(value->IsUndefined());
  }

  BOOST_AUTO_TEST_CASE(
    get_property_result_should_remain_valid_after_parent_object_is_destroyed)
  {
    qjs::Value property = [&]() {
      auto object = qjs::Value::Object(context);
      BOOST_REQUIRE(object.has_value());

      auto value = qjs::Value::String(context, "test");
      BOOST_REQUIRE(value.has_value());

      auto set_result = object->SetProperty("name", std::move(*value));
      BOOST_REQUIRE(set_result.has_value());

      auto got = object->GetProperty("name");
      BOOST_REQUIRE(got.has_value());
      BOOST_REQUIRE(got->IsValid());
      BOOST_REQUIRE(got->IsString());

      return std::move(*got);
    }();

    // obj has been destroyed here, so property should still remain valid.

    BOOST_TEST(property.IsValid());
    BOOST_TEST(property.IsString());

    auto text = property.ToString();
    BOOST_REQUIRE(text.has_value());
    BOOST_TEST(*text == "test");
  }

  BOOST_AUTO_TEST_CASE(set_property_twice_should_overwrite_existing_value)
  {
    auto obj = qjs::Value::Object(context);
    BOOST_REQUIRE(obj.has_value());
    BOOST_REQUIRE(obj->IsValid());

    auto first = qjs::Value::String(context, "first");
    BOOST_REQUIRE(first.has_value());
    BOOST_REQUIRE(first->IsValid());

    auto second = qjs::Value::String(context, "second");
    BOOST_REQUIRE(second.has_value());
    BOOST_REQUIRE(second->IsValid());

    auto set_result = obj->SetProperty("name", std::move(*first));
    BOOST_REQUIRE(set_result.has_value());
    set_result = obj->SetProperty("name", std::move(*second));
    BOOST_REQUIRE(set_result.has_value());

    auto got = obj->GetProperty("name");
    BOOST_REQUIRE(got.has_value());
    BOOST_TEST(got->IsValid());
    BOOST_REQUIRE(got->IsString());

    auto test = got->ToString();
    BOOST_REQUIRE(test.has_value());
    BOOST_TEST(*test == "second");
  }

  BOOST_AUTO_TEST_CASE(
    get_property_result_should_keep_old_value_after_property_is_overwritten)
  {
    auto object = qjs::Value::Object(context);
    BOOST_REQUIRE(object.has_value());
    BOOST_REQUIRE(object->IsValid());

    auto first = qjs::Value::String(context, "first");
    BOOST_REQUIRE(first.has_value());
    BOOST_REQUIRE(first->IsValid());

    auto set_result = object->SetProperty("name", std::move(*first));
    BOOST_REQUIRE(set_result.has_value());

    auto got_first = object->GetProperty("name");
    BOOST_REQUIRE(got_first.has_value());
    BOOST_REQUIRE(got_first->IsValid());

    // SetProperty again
    auto second = qjs::Value::String(context, "second");
    BOOST_REQUIRE(second.has_value());
    BOOST_REQUIRE(second->IsValid());
    set_result = object->SetProperty("name", std::move(*second));
    BOOST_REQUIRE(set_result.has_value());

    BOOST_REQUIRE(got_first.has_value());
    BOOST_REQUIRE(got_first->IsValid());
    BOOST_REQUIRE(got_first->IsString());
    const auto first_text = got_first->ToString();
    BOOST_REQUIRE(first_text.has_value());
    BOOST_TEST(*first_text == "first");

    auto got_second = object->GetProperty("name");
    BOOST_REQUIRE(got_second.has_value());
    BOOST_REQUIRE(got_second->IsValid());
    const auto second_text = got_second->ToString();
    BOOST_REQUIRE(second_text.has_value());
    BOOST_TEST(*second_text == "second");
  }

  BOOST_AUTO_TEST_CASE(set_nested_object_property_should_preserve_nested_object)
  {
    auto parent = qjs::Value::Object(context);
    BOOST_REQUIRE(parent.has_value());
    BOOST_REQUIRE(parent->IsValid());
    BOOST_REQUIRE(parent->IsObject());

    auto child = qjs::Value::Object(context);
    BOOST_REQUIRE(child.has_value());
    BOOST_REQUIRE(child->IsValid());
    BOOST_REQUIRE(child->IsObject());

    auto answer = qjs::Value::Int32(context, 42);
    BOOST_REQUIRE(answer.has_value());
    BOOST_REQUIRE(answer->IsValid());

    auto set_result = child->SetProperty("answer", std::move(*answer));
    BOOST_REQUIRE(set_result.has_value());
    BOOST_TEST(!answer->IsValid());

    set_result = parent->SetProperty("child", std::move(*child));
    BOOST_REQUIRE(set_result.has_value());
    BOOST_TEST(!child->IsValid());

    auto got_child = parent->GetProperty("child");
    BOOST_REQUIRE(got_child.has_value());
    BOOST_REQUIRE(got_child->IsValid());
    BOOST_TEST(got_child->IsObject());

    auto got_answer = got_child->GetProperty("answer");
    BOOST_REQUIRE(got_answer.has_value());
    BOOST_REQUIRE(got_answer->IsValid());
    BOOST_TEST(got_answer->IsNumber());

    auto result = got_answer->ToInt32();
    BOOST_REQUIRE(result.has_value());
    BOOST_TEST(*result == 42);
  }

  BOOST_AUTO_TEST_CASE(
    set_property_with_value_from_different_context_should_return_invalid_argument)
  {
    QjsContextFixture lhs_fixture;
    QjsContextFixture rhs_fixture;

    auto target = qjs::Value::Object(lhs_fixture.context);
    BOOST_REQUIRE(target.has_value());
    BOOST_REQUIRE(target->IsValid());
    BOOST_REQUIRE(target->IsObject());

    auto value = qjs::Value::String(rhs_fixture.context, "cross-context");
    BOOST_REQUIRE(value.has_value());
    BOOST_REQUIRE(value->IsValid());
    BOOST_REQUIRE(value->IsString());

    auto set_result = target->SetProperty("message", std::move(*value));
    RequireErrorCode(set_result, Errc::InvalidArgument);

    // SetProperty consumes the input Value regardless of success or failure.
    BOOST_TEST(!value->IsValid());

    auto got = target->GetProperty("message");
    BOOST_REQUIRE(got.has_value());
    BOOST_TEST(got->IsUndefined());
  }

  BOOST_AUTO_TEST_SUITE_END()

  BOOST_FIXTURE_TEST_SUITE(invalid_context_tests, QjsContextFixture)

  BOOST_AUTO_TEST_CASE(create_undefined_value_with_invalid_context_should_fail)
  {
    qjs::Context moved = qjs::Context { std::move(context) };
    auto value = qjs::Value::Undefined(context);
    RequireErrorCode(value, Errc::InvalidArgument);
  }

  BOOST_AUTO_TEST_CASE(create_null_with_invalid_context_should_fail)
  {
    qjs::Context moved = qjs::Context { std::move(context) };
    auto value = qjs::Value::Null(context);
    RequireErrorCode(value, Errc::InvalidArgument);
  }

  BOOST_AUTO_TEST_CASE(create_bool_with_invalid_context_should_fail)
  {
    qjs::Context moved = qjs::Context { std::move(context) };
    auto value = qjs::Value::Bool(context, true);
    RequireErrorCode(value, Errc::InvalidArgument);
  }

  BOOST_AUTO_TEST_CASE(create_int32_with_invalid_context_should_fail)
  {
    qjs::Context moved = qjs::Context { std::move(context) };
    auto value = qjs::Value::Int32(context, 42);
    RequireErrorCode(value, Errc::InvalidArgument);
  }

  BOOST_AUTO_TEST_CASE(create_string_with_invalid_context_should_fail)
  {
    qjs::Context moved = qjs::Context { std::move(context) };
    auto value = qjs::Value::String(context, "test");
    RequireErrorCode(value, Errc::InvalidArgument);
  }

  BOOST_AUTO_TEST_CASE(create_object_with_invalid_context_should_fail)
  {
    qjs::Context moved = qjs::Context { std::move(context) };
    auto value = qjs::Value::Object(context);
    RequireErrorCode(value, Errc::InvalidArgument);
  }

  BOOST_AUTO_TEST_SUITE_END()

  BOOST_FIXTURE_TEST_SUITE(invalid_value_tests, QjsContextFixture)

  BOOST_AUTO_TEST_CASE(moved_from_value_to_bool_should_fail)
  {
    auto value = qjs::Value::Bool(context, true);
    BOOST_REQUIRE(value.has_value());
    BOOST_REQUIRE(value->IsValid());

    auto moved = std::move(*value);

    RequireErrorCode(value->ToBool(), Errc::InvalidArgument);
  }

  BOOST_AUTO_TEST_CASE(moved_from_value_to_int32_should_fail)
  {
    auto value = qjs::Value::Int32(context, 42);
    BOOST_REQUIRE(value.has_value());
    BOOST_REQUIRE(value->IsValid());

    auto moved = std::move(*value);

    RequireErrorCode(value->ToInt32(), Errc::InvalidArgument);
  }

  BOOST_AUTO_TEST_CASE(moved_from_value_to_string_should_fail)
  {
    auto value = qjs::Value::String(context, "test");
    BOOST_REQUIRE(value.has_value());
    BOOST_REQUIRE(value->IsValid());

    auto moved = std::move(*value);

    RequireErrorCode(value->ToString(), Errc::InvalidArgument);
  }

  BOOST_AUTO_TEST_CASE(moved_from_value_get_property_should_fail)
  {
    auto value = qjs::Value::String(context, "test");
    BOOST_REQUIRE(value.has_value());
    BOOST_REQUIRE(value->IsValid());
    BOOST_REQUIRE(value->IsString());

    auto moved = std::move(*value);

    RequireErrorCode(value->GetProperty("length"), Errc::InvalidArgument);
  }

  BOOST_AUTO_TEST_CASE(set_property_with_moved_from_target_should_fail)
  {
    auto target = qjs::Value::Object(context);
    BOOST_REQUIRE(target.has_value());
    BOOST_REQUIRE(target->IsValid());

    auto value = qjs::Value::String(context, "test");
    BOOST_REQUIRE(value.has_value());
    BOOST_REQUIRE(value->IsValid());

    // move target
    auto moved = std::move(*target);

    auto set_result = target->SetProperty("name", std::move(*value));
    RequireErrorCode(set_result, Errc::InvalidArgument);
  }

  BOOST_AUTO_TEST_CASE(set_property_with_moved_from_value_should_fail)
  {
    auto target = qjs::Value::Object(context);
    BOOST_REQUIRE(target.has_value());
    BOOST_REQUIRE(target->IsValid());

    auto value = qjs::Value::String(context, "test");
    BOOST_REQUIRE(value.has_value());
    BOOST_REQUIRE(value->IsValid());

    // move value
    auto moved = std::move(*value);

    auto set_result = target->SetProperty("message", std::move(*value));
    RequireErrorCode(set_result, Errc::InvalidArgument);
  }

  BOOST_AUTO_TEST_CASE(
    set_property_with_moved_from_value_fail_should_not_affect_target)
  {
    auto target = qjs::Value::Object(context);
    BOOST_REQUIRE(target.has_value());
    BOOST_REQUIRE(target->IsValid());

    auto value = qjs::Value::String(context, "test");
    BOOST_REQUIRE(value.has_value());
    BOOST_REQUIRE(value->IsValid());

    auto moved = std::move(*value);
    auto set_result = target->SetProperty("message", std::move(*value));
    RequireErrorCode(set_result, Errc::InvalidArgument);

    // to test that target still valid
    BOOST_TEST(target->IsValid());
    auto got = target->GetProperty("message");
    BOOST_REQUIRE(got.has_value());
    BOOST_REQUIRE(got->IsValid());
    BOOST_TEST(got->IsUndefined());
  }

  BOOST_AUTO_TEST_SUITE_END()

  BOOST_FIXTURE_TEST_SUITE(exception_path_tests, QjsContextFixture)

  BOOST_AUTO_TEST_CASE(
    to_string_when_js_to_string_throws_should_return_to_string_failed)
  {
    auto value = qjs::Evaluator::Eval(
      context, "({ toString() { throw new Error('toString failed'); } })");
    BOOST_REQUIRE(value.has_value());
    BOOST_REQUIRE(value->IsValid());

    auto result = value->ToString();
    RequireErrorCode(result, Errc::ToStringFailed);
    RequireErrorMessageContains(result, "toString failed");
  }

  BOOST_AUTO_TEST_CASE(
    to_int32_when_js_value_of_throws_should_return_to_int32_failed)
  {
    auto value = qjs::Evaluator::Eval(
      context, "({ valueOf() { throw new Error('valueOf failed'); } })");
    BOOST_REQUIRE(value.has_value());
    BOOST_REQUIRE(value->IsValid());

    auto result = value->ToInt32();
    RequireErrorCode(result, Errc::ToIntFailed);
    RequireErrorMessageContains(result, "valueOf failed");
  }

  BOOST_AUTO_TEST_CASE(
    get_property_when_js_getter_throws_should_return_get_property_failed)
  {
    auto value = qjs::Evaluator::Eval(
      context, "({ get boom() { throw new Error('getter failed'); } })");
    BOOST_REQUIRE(value.has_value());
    BOOST_REQUIRE(value->IsValid());
    BOOST_REQUIRE(value->IsObject());

    auto result = value->GetProperty("boom");
    RequireErrorCode(result, Errc::GetPropertyFailed);
    RequireErrorMessageContains(result, "getter failed");
  }

  BOOST_AUTO_TEST_CASE(
    set_property_when_js_setter_throws_should_return_set_property_failed)
  {
    auto object = qjs::Evaluator::Eval(
      context, "({ set boom(value) { throw new Error('setter failed'); } })");
    BOOST_REQUIRE(object.has_value());
    BOOST_REQUIRE(object->IsValid());
    BOOST_REQUIRE(object->IsObject());

    auto value = qjs::Value::String(context, "test");
    BOOST_REQUIRE(value.has_value());
    BOOST_REQUIRE(value->IsValid());

    auto set_result = object->SetProperty("boom", std::move(*value));
    RequireErrorCode(set_result, Errc::SetPropertyFailed);
    RequireErrorMessageContains(set_result, "setter failed");
  }

  BOOST_AUTO_TEST_SUITE_END()

  BOOST_FIXTURE_TEST_SUITE(array_value_tests, QjsContextFixture)

  BOOST_AUTO_TEST_CASE(array_value_should_be_valid_array)
  {
    auto value = qjs::Value::Array(context);
    BOOST_REQUIRE(value.has_value());
    BOOST_REQUIRE(value->IsValid());

    BOOST_TEST(value->IsArray());
    BOOST_TEST(value->IsObject());

    BOOST_TEST(!value->IsUndefined());
    BOOST_TEST(!value->IsNull());
    BOOST_TEST(!value->IsNumber());
    BOOST_TEST(!value->IsString());
    BOOST_TEST(!value->IsBool());
  }

  BOOST_AUTO_TEST_CASE(empty_array_should_have_zero_length)
  {
    auto value = qjs::Value::Array(context);
    BOOST_REQUIRE(value.has_value());
    BOOST_REQUIRE(value->IsValid());
    BOOST_REQUIRE(value->IsArray());

    const auto length = value->ArrayLength();
    BOOST_REQUIRE(length.has_value());
    BOOST_TEST(*length == 0);
  }

  BOOST_AUTO_TEST_CASE(
    set_element_with_string_value_should_store_same_string_value)
  {
    auto array = qjs::Value::Array(context);
    BOOST_REQUIRE(array.has_value());
    BOOST_REQUIRE(array->IsValid());
    BOOST_REQUIRE(array->IsArray());

    auto value = qjs::Value::String(context, "proxy++");
    BOOST_REQUIRE(value.has_value());
    BOOST_REQUIRE(value->IsValid());

    const auto set_result = array->SetElement(0, std::move(*value));
    BOOST_REQUIRE(set_result.has_value());

    BOOST_TEST(!value->IsValid());

    auto got = array->GetElement(0);
    BOOST_REQUIRE(got.has_value());
    BOOST_REQUIRE(got->IsValid());
    BOOST_REQUIRE(got->IsString());

    auto text = got->ToString();
    BOOST_REQUIRE(text.has_value());
    BOOST_TEST(*text == "proxy++");
  }

  BOOST_AUTO_TEST_CASE(set_element_should_update_array_length)
  {
    auto array = qjs::Value::Array(context);
    BOOST_REQUIRE(array.has_value());
    BOOST_REQUIRE(array->IsValid());
    BOOST_REQUIRE(array->IsArray());

    auto value = qjs::Value::Int32(context, 42);
    BOOST_REQUIRE(value.has_value());
    BOOST_REQUIRE(value->IsValid());

    const auto set_result = array->SetElement(2, std::move(*value));
    BOOST_REQUIRE(set_result.has_value());
    BOOST_TEST(!value->IsValid());

    auto length = array->ArrayLength();
    BOOST_REQUIRE(length.has_value());
    BOOST_TEST(*length == 3);
  }

  BOOST_AUTO_TEST_CASE(get_missing_element_should_return_undefined)
  {
    auto array = qjs::Value::Array(context);
    BOOST_REQUIRE(array.has_value());
    BOOST_REQUIRE(array->IsValid());
    BOOST_REQUIRE(array->IsArray());

    auto value = qjs::Value::Int32(context, 42);
    BOOST_REQUIRE(value.has_value());
    BOOST_REQUIRE(value->IsValid());

    const auto set_result = array->SetElement(2, std::move(*value));
    BOOST_REQUIRE(set_result.has_value());
    BOOST_TEST(!value->IsValid());

    auto first_value = array->GetElement(0);
    BOOST_REQUIRE(first_value.has_value());
    BOOST_TEST(first_value->IsUndefined());

    auto second_value = array->GetElement(1);
    BOOST_REQUIRE(second_value.has_value());
    BOOST_TEST(second_value->IsUndefined());

    auto third_value = array->GetElement(2);
    BOOST_REQUIRE(third_value.has_value());
    BOOST_REQUIRE(third_value->IsNumber());

    auto number = third_value->ToInt32();
    BOOST_REQUIRE(number.has_value());
    BOOST_TEST(*number == 42);
  }

  BOOST_AUTO_TEST_CASE(set_property_with_array_should_preserve_array_elements)
  {
    auto object = qjs::Value::Object(context);
    BOOST_REQUIRE(object.has_value());
    BOOST_REQUIRE(object->IsValid());
    BOOST_REQUIRE(object->IsObject());

    auto array = qjs::Value::Array(context);
    BOOST_REQUIRE(array.has_value());
    BOOST_REQUIRE(array->IsValid());
    BOOST_REQUIRE(array->IsArray());

    auto value = qjs::Value::String(context, "application/json");
    BOOST_REQUIRE(value.has_value());
    BOOST_REQUIRE(value->IsValid());
    BOOST_REQUIRE(value->IsString());

    auto set_elem_result = array->SetElement(0, std::move(*value));
    BOOST_REQUIRE(set_elem_result.has_value());

    auto set_property_result
      = object->SetProperty("accept", std::move(*array));
    BOOST_REQUIRE(set_property_result.has_value());

    auto got_array = object->GetProperty("accept");
    BOOST_REQUIRE(got_array.has_value());
    BOOST_REQUIRE(got_array->IsValid());
    BOOST_REQUIRE(got_array->IsArray());

    auto got_value = got_array->GetElement(0);
    BOOST_REQUIRE(got_value.has_value());
    BOOST_REQUIRE(got_value->IsValid());
    BOOST_REQUIRE(got_value->IsString());

    auto text = got_value->ToString();
    BOOST_REQUIRE(text.has_value());
    BOOST_TEST(*text == "application/json");
  }

  BOOST_AUTO_TEST_CASE(create_array_with_invalid_context_should_fail)
  {
    qjs::Context moved_context = qjs::Context { std::move(context) };
    boost::ignore_unused(moved_context);
    auto array = qjs::Value::Array(context);
    RequireErrorCode(array, Errc::InvalidArgument);
  }

  BOOST_AUTO_TEST_CASE(moved_from_value_get_element_should_fail)
  {
    auto array = qjs::Value::Array(context);
    BOOST_REQUIRE(array.has_value());
    BOOST_REQUIRE(array->IsValid());
    BOOST_REQUIRE(array->IsArray());

    auto moved = std::move(*array);
    boost::ignore_unused(moved);

    RequireErrorCode(array->GetElement(0), Errc::InvalidArgument);
  }

  BOOST_AUTO_TEST_CASE(moved_from_value_array_length_should_fail)
  {
    auto array = qjs::Value::Array(context);
    BOOST_REQUIRE(array.has_value());
    BOOST_REQUIRE(array->IsValid());
    BOOST_REQUIRE(array->IsArray());

    auto moved = std::move(*array);
    boost::ignore_unused(moved);

    RequireErrorCode(array->ArrayLength(), Errc::InvalidArgument);
  }

  BOOST_AUTO_TEST_CASE(set_element_with_moved_from_value_should_fail)
  {
    auto array = qjs::Value::Array(context);
    BOOST_REQUIRE(array.has_value());
    BOOST_REQUIRE(array->IsValid());
    BOOST_REQUIRE(array->IsArray());

    auto moved = std::move(*array);
    boost::ignore_unused(moved);

    auto value = qjs::Value::Int32(context, 42);
    BOOST_REQUIRE(value.has_value());
    BOOST_REQUIRE(value->IsValid());
    BOOST_REQUIRE(value->IsNumber());

    RequireErrorCode(array->SetElement(0, std::move(*value)),
                     Errc::InvalidArgument);
    BOOST_TEST(!value->IsValid());
  }

  BOOST_AUTO_TEST_SUITE_END()

}