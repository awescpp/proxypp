/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#define BOOST_TEST_MODULE test_qjs_object_builder

#include "proxypp/script/qjs.h"
#include "qjs_fixture.h"
#include <boost/test/unit_test.hpp>

namespace proxypp::script::qjs::test
{

  BOOST_FIXTURE_TEST_SUITE(object_builder_tests, QjsContextFixture)

  BOOST_AUTO_TEST_CASE(object_builder_should_create_valid_object)
  {
    auto builder = ObjectBuilder::Create(context);
    BOOST_REQUIRE(builder.has_value());

    auto request = std::move(builder->SetString("method", "GET")).Build();
    BOOST_REQUIRE(request.has_value());
    BOOST_TEST(request->IsValid());
    BOOST_TEST(request->IsObject());
  }

  BOOST_AUTO_TEST_CASE(object_builder_set_string_should_store_same_string)
  {
    auto builder = ObjectBuilder::Create(context);
    BOOST_REQUIRE(builder.has_value());

    auto request = std::move(builder->SetString("method", "GET")).Build();
    BOOST_REQUIRE(request.has_value());
    BOOST_TEST(request->IsValid());
    BOOST_TEST(request->IsObject());

    auto property = request->GetProperty("method");
    BOOST_REQUIRE(property.has_value());
    BOOST_REQUIRE(property->IsValid());
    BOOST_REQUIRE(property->IsString());

    auto text = property->ToString();
    BOOST_REQUIRE(text.has_value());
    BOOST_TEST(*text == "GET");
  }

  BOOST_AUTO_TEST_CASE(object_builder_set_int32_should_store_same_int32)
  {
    auto builder = ObjectBuilder::Create(context);
    BOOST_REQUIRE(builder.has_value());

    auto request
      = std::move(builder->SetInt32("Content-Length", 8848)).Build();
    BOOST_REQUIRE(request.has_value());
    BOOST_TEST(request->IsValid());
    BOOST_TEST(request->IsObject());

    auto property = request->GetProperty("Content-Length");
    BOOST_REQUIRE(property.has_value());
    BOOST_REQUIRE(property->IsValid());
    BOOST_REQUIRE(property->IsNumber());

    auto number = property->ToInt32();
    BOOST_REQUIRE(number.has_value());
    BOOST_TEST(*number == 8848);
  }

  BOOST_AUTO_TEST_CASE(object_builder_set_bool_should_store_same_bool)
  {
    auto builder = ObjectBuilder::Create(context);
    BOOST_REQUIRE(builder.has_value());

    auto request = std::move(builder->SetBool("hasCookie", true)).Build();
    BOOST_REQUIRE(request.has_value());
    BOOST_TEST(request->IsValid());
    BOOST_TEST(request->IsObject());

    auto property = request->GetProperty("hasCookie");
    BOOST_REQUIRE(property.has_value());
    BOOST_REQUIRE(property->IsValid());
    BOOST_REQUIRE(property->IsBool());

    auto bool_value = property->ToBool();
    BOOST_REQUIRE(bool_value.has_value());
    BOOST_TEST(*bool_value == true);
  }

  BOOST_AUTO_TEST_CASE(object_builder_set_null_should_store_null)
  {
    auto builder = ObjectBuilder::Create(context);
    BOOST_REQUIRE(builder.has_value());

    auto request = std::move(builder->SetNull("data")).Build();
    BOOST_REQUIRE(request.has_value());
    BOOST_TEST(request->IsValid());
    BOOST_TEST(request->IsObject());

    auto property = request->GetProperty("data");
    BOOST_REQUIRE(property.has_value());
    BOOST_REQUIRE(property->IsValid());
    BOOST_TEST(property->IsNull());
  }

  BOOST_AUTO_TEST_CASE(object_builder_set_object_should_preserve_nested_object)
  {
    auto nested_obj
      = std::move(ObjectBuilder::Create(context)->SetString("message", "ok"))
          .Build();
    BOOST_REQUIRE(nested_obj.has_value());
    BOOST_REQUIRE(nested_obj->IsValid());
    BOOST_REQUIRE(nested_obj->IsObject());

    auto obj = std::move(ObjectBuilder::Create(context)->SetObject(
                           "nested_obj", std::move(*nested_obj)))
                 .Build();
    BOOST_REQUIRE(obj.has_value());
    BOOST_REQUIRE(obj->IsValid());
    BOOST_REQUIRE(obj->IsObject());

    auto nested = obj->GetProperty("nested_obj");
    BOOST_REQUIRE(nested.has_value());
    BOOST_REQUIRE(nested->IsValid());
    BOOST_REQUIRE(nested->IsObject());

    auto value = nested->GetProperty("message");
    BOOST_REQUIRE(value.has_value());

    auto text = value->ToString();
    BOOST_REQUIRE(text.has_value());
    BOOST_TEST(*text == "ok");
  }

  BOOST_AUTO_TEST_CASE(object_builder_set_array_should_preserve_array_elements)
  {
    auto first_builder = ObjectBuilder::Create(context);
    BOOST_REQUIRE(first_builder.has_value());

    auto first_item
      = std::move(first_builder->SetString("name", "foo")).Build();
    BOOST_REQUIRE(first_item.has_value());
    BOOST_REQUIRE(first_item->IsValid());
    BOOST_REQUIRE(first_item->IsObject());

    auto second_builder = ObjectBuilder::Create(context);
    BOOST_REQUIRE(second_builder.has_value());

    auto second_item
      = std::move(second_builder->SetString("name", "bar")).Build();
    BOOST_REQUIRE(second_item.has_value());
    BOOST_REQUIRE(second_item->IsValid());
    BOOST_REQUIRE(second_item->IsObject());

    auto array = qjs::Value::Array(context);
    BOOST_REQUIRE(array.has_value());
    BOOST_REQUIRE(array->IsValid());
    BOOST_REQUIRE(array->IsArray());

    auto set_result = array->SetElement(0, std::move(*first_item));
    BOOST_REQUIRE(set_result.has_value());

    set_result = array->SetElement(1, std::move(*second_item));
    BOOST_REQUIRE(set_result.has_value());

    auto builder = ObjectBuilder::Create(context);
    BOOST_REQUIRE(builder.has_value());

    auto object
      = std::move(builder->SetValue("items", std::move(*array))).Build();
    BOOST_REQUIRE(object.has_value());
    BOOST_REQUIRE(object->IsValid());
    BOOST_REQUIRE(object->IsObject());

    auto items = object->GetProperty("items");
    BOOST_REQUIRE(items.has_value());
    BOOST_REQUIRE(items->IsValid());
    BOOST_REQUIRE(items->IsArray());

    auto length = items->ArrayLength();
    BOOST_REQUIRE(length.has_value());
    BOOST_TEST(*length == 2);

    auto first = items->GetElement(0);
    BOOST_REQUIRE(first.has_value());
    BOOST_REQUIRE(first->IsValid());
    BOOST_REQUIRE(first->IsObject());

    auto first_name = first->GetProperty("name");
    BOOST_REQUIRE(first_name.has_value());
    BOOST_REQUIRE(first_name->IsValid());
    BOOST_REQUIRE(first_name->IsString());

    auto first_text = first_name->ToString();
    BOOST_REQUIRE(first_text.has_value());
    BOOST_TEST(*first_text == "foo");

    auto second = items->GetElement(1);
    BOOST_REQUIRE(second.has_value());
    BOOST_REQUIRE(second->IsValid());
    BOOST_REQUIRE(second->IsObject());

    auto second_name = second->GetProperty("name");
    BOOST_REQUIRE(second_name.has_value());
    BOOST_REQUIRE(second_name->IsValid());
    BOOST_REQUIRE(second_name->IsString());

    auto second_text = second_name->ToString();
    BOOST_REQUIRE(second_text.has_value());
    BOOST_TEST(*second_text == "bar");
  }

  BOOST_AUTO_TEST_CASE(object_builder_set_string_array_should_store_all_strings)
  {
    std::vector<std::string> vec { "foo", "bar", "baz" };

    auto array = qjs::Value::Array(context);
    Result<void> set_result = {};
    for(int i = 0; i < vec.size(); ++i)
      {
        auto value = qjs::Value::String(context, vec[i]);
        BOOST_REQUIRE(value.has_value());
        BOOST_REQUIRE(value->IsValid());
        set_result = array->SetElement(i, std::move(*value));
        BOOST_REQUIRE(set_result.has_value());
      }

    auto builder = ObjectBuilder::Create(context);
    BOOST_REQUIRE(builder.has_value());

    auto object
      = std::move(builder->SetValue("arr", std::move(*array))).Build();
    BOOST_REQUIRE(object.has_value());
    BOOST_REQUIRE(object->IsValid());
    BOOST_REQUIRE(object->IsObject());

    auto property_arr = object->GetProperty("arr");
    BOOST_REQUIRE(property_arr.has_value());
    BOOST_REQUIRE(property_arr->IsValid());
    BOOST_REQUIRE(property_arr->IsArray());

    auto arr_length = property_arr->ArrayLength();
    BOOST_REQUIRE(arr_length.has_value());
    BOOST_TEST(*arr_length == 3);
    for(int i = 0; i < vec.size(); ++i)
      {
        auto value = property_arr->GetElement(i);
        BOOST_REQUIRE(value.has_value());
        BOOST_REQUIRE(value->IsString());
        auto text = value->ToString();
        BOOST_REQUIRE(text.has_value());
        BOOST_TEST(*text == vec[i]);
      }
  }

  BOOST_AUTO_TEST_CASE(
    object_builder_build_should_consume_builder_or_return_valid_object)
  {
    static_assert(
      !std::is_invocable_v<decltype(&ObjectBuilder::Build), ObjectBuilder&>);

    static_assert(
      std::is_invocable_v<decltype(&ObjectBuilder::Build), ObjectBuilder&&>);

    auto builder = ObjectBuilder::Create(context);
    BOOST_REQUIRE(builder.has_value());

    builder->SetString("method", "POST")
      .SetBool("hasBody", true)
      .SetInt32("contentLength", 12);

    auto object = std::move(*builder).Build();
    BOOST_REQUIRE(object.has_value());
    BOOST_REQUIRE(object->IsValid());
    BOOST_REQUIRE(object->IsObject());

    auto method = object->GetProperty("method");
    BOOST_REQUIRE(method.has_value());
    BOOST_REQUIRE(method->IsString());

    auto method_text = method->ToString();
    BOOST_REQUIRE(method_text.has_value());
    BOOST_TEST(*method_text == "POST");

    auto has_body = object->GetProperty("hasBody");
    BOOST_REQUIRE(has_body.has_value());
    BOOST_REQUIRE(has_body->IsBool());

    auto has_body_value = has_body->ToBool();
    BOOST_REQUIRE(has_body_value.has_value());
    BOOST_TEST(*has_body_value == true);

    auto content_length = object->GetProperty("contentLength");
    BOOST_REQUIRE(content_length.has_value());
    BOOST_REQUIRE(content_length->IsNumber());

    auto content_length_value = content_length->ToInt32();
    BOOST_REQUIRE(content_length_value.has_value());
    BOOST_TEST(*content_length_value == 12);
  }

  BOOST_AUTO_TEST_CASE(object_builder_with_invalid_context_should_fail)
  {
    auto moved_to = std::move(context);
    boost::ignore_unused(moved_to);

    auto builder = ObjectBuilder::Create(context);
    BOOST_REQUIRE(!builder.has_value());
    RequireErrorCode(builder, Errc::InternalError);
  }

  BOOST_AUTO_TEST_SUITE_END()

}