/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#define BOOST_TEST_MODULE QuickJSApiTest

#include <boost/test/unit_test.hpp>
#include <quickjs.h>

struct JsRuntimeDeleter
{
  void operator()(JSRuntime* runtime) const noexcept
  {
    if(runtime != nullptr)
      {
        JS_FreeRuntime(runtime);
      }
  }
};

struct JsContextDeleter
{
  void operator()(JSContext* context) const noexcept
  {
    if(context != nullptr)
      {
        JS_FreeContext(context);
      }
  }
};

class QuickJsFixture
{
public:
  QuickJsFixture() : runtime_(JS_NewRuntime()), context_(nullptr)
  {
    if(!runtime_)
      {
        throw std::runtime_error("JS_NewRuntime() failed");
      }
    context_.reset(JS_NewContext(runtime_.get()));
    if(!context_)
      {
        throw std::runtime_error("JS_NewContext() failed");
      }
  }

  QuickJsFixture(const QuickJsFixture&) = delete;
  QuickJsFixture& operator=(const QuickJsFixture&) = delete;
  QuickJsFixture(QuickJsFixture&&) = delete;
  QuickJsFixture& operator=(QuickJsFixture&&) = delete;

  JSRuntime* runtime() noexcept { return runtime_.get(); }

  JSContext* context() noexcept { return context_.get(); }

  std::string GetExceptionMessage()
  {
    const JSValue ex = JS_GetException(context());
    const char* message = JS_ToCString(context(), ex);
    std::string result
      = message != nullptr ? message : "<cannot convert exception to string>";
    JS_FreeCString(context(), message);
    JS_FreeValue(context(), ex);
    return result;
  }

private:
  std::unique_ptr<JSRuntime, JsRuntimeDeleter> runtime_;
  std::unique_ptr<JSContext, JsContextDeleter> context_;
};

BOOST_FIXTURE_TEST_SUITE(quick_js_basic_test_suite, QuickJsFixture)

BOOST_AUTO_TEST_CASE(eval_arithmetic_expression_should_return_expected_result)
{
  const std::string_view expr = "1 + 2 + 3 + 4";
  JSValue result = JS_Eval(context(), expr.data(), expr.size(),
                           "<quickjs-test>", JS_EVAL_TYPE_GLOBAL);
  if(JS_IsException(result))
    {
      JS_FreeValue(context(), result);
      BOOST_FAIL("JavaScript exception: " << GetExceptionMessage());
    }

  const char* str = JS_ToCString(context(), result);
  BOOST_REQUIRE(str != nullptr);
  BOOST_TEST(std::string(str) == "10");
  JS_FreeCString(context(), str);
  JS_FreeValue(context(), result);
}

BOOST_AUTO_TEST_SUITE_END()