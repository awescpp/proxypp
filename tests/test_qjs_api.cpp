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
        runtime = nullptr;
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
        context = nullptr;
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

  void InjectContext()
  {
    JSValue global_obj_ref = JS_GetGlobalObject(context());

    JSValue ctx_obj = JS_NewObject(context());
    if(JS_IsException(ctx_obj))
      {
        JS_FreeValue(context(), global_obj_ref);
        BOOST_FAIL("JS_NewObject(ctx) failed " << GetExceptionMessage());
      }

    JSValue request_obj = JS_NewObject(context());
    if(JS_IsException(request_obj))
      {
        JS_FreeValue(context(), ctx_obj);
        JS_FreeValue(context(), global_obj_ref);
        BOOST_FAIL("JS_NewObject(request_obj) failed "
                   << GetExceptionMessage());
      }

    JSValue host = JS_NewString(context(), "example.com");
    if(JS_IsException(host))
      {
        JS_FreeValue(context(), request_obj);
        JS_FreeValue(context(), ctx_obj);
        JS_FreeValue(context(), global_obj_ref);
        BOOST_FAIL("JS_NewString(host) failed " << GetExceptionMessage());
      }

    JSValue method = JS_NewString(context(), "GET");
    if(JS_IsException(method))
      {
        JS_FreeValue(context(), host);
        JS_FreeValue(context(), request_obj);
        JS_FreeValue(context(), ctx_obj);
        JS_FreeValue(context(), global_obj_ref);
        BOOST_FAIL("JS_NewString(method) failed " << GetExceptionMessage());
      }

    // Regardless of whether the call succeeds, QuickJS has taken
    // ownership of `host`, so `host` no longer needs to be released manually
    // If `host` is a `JSValueConst`, it must be released manually,
    // because QuickJS does not take ownership of `JSValueConst`
    if(JS_SetPropertyStr(context(), request_obj, "host", host) < 0)
      {
        JS_FreeValue(context(), method);
        JS_FreeValue(context(), request_obj);
        JS_FreeValue(context(), ctx_obj);
        JS_FreeValue(context(), global_obj_ref);
        BOOST_FAIL("set request.host failed  " << GetExceptionMessage());
      }

    if(JS_SetPropertyStr(context(), request_obj, "method", method) < 0)
      {
        JS_FreeValue(context(), request_obj);
        JS_FreeValue(context(), ctx_obj);
        JS_FreeValue(context(), global_obj_ref);
        BOOST_FAIL("set request.method failed  " << GetExceptionMessage());
      }

    if(JS_SetPropertyStr(context(), ctx_obj, "request", request_obj) < 0)
      {
        JS_FreeValue(context(), ctx_obj);
        JS_FreeValue(context(), global_obj_ref);
        BOOST_FAIL("set ctx.request failed  " << GetExceptionMessage());
      }

    if(JS_SetPropertyStr(context(), global_obj_ref, "ctx", ctx_obj) < 0)
      {
        JS_FreeValue(context(), global_obj_ref);
        BOOST_FAIL("set global.ctx failed  " << GetExceptionMessage());
      }

    JS_FreeValue(context(), global_obj_ref);
  }

  bool EvalBooleanExpression(std::string_view expr)
  {
    JSValue result = JS_Eval(context(), expr.data(), expr.size(),
                             "<evel-boolean>", JS_EVAL_TYPE_GLOBAL);
    if(JS_IsException(result))
      {
        JS_FreeValue(context(), result);
        throw std::runtime_error(
          std::format("JS_Eval failed, {}", GetExceptionMessage()));
      }
    const int matched = JS_ToBool(context(), result);
    if(matched < 0)
      {
        JS_FreeValue(context(), result);
        throw std::runtime_error(
          std::format("JS_ToBool failed, {}", GetExceptionMessage()));
      }
    JS_FreeValue(context(), result);
    return matched == 1;
  }

private:
  std::unique_ptr<JSRuntime, JsRuntimeDeleter> runtime_;
  std::unique_ptr<JSContext, JsContextDeleter> context_;
};

BOOST_FIXTURE_TEST_SUITE(quick_js_basic_test_suite, QuickJsFixture)

BOOST_AUTO_TEST_CASE(eval_arithmetic_expression_should_return_expected_result)
{
  const std::string_view expr = "1 + 2 + 3 + 4";
  const JSValue result = JS_Eval(context(), expr.data(), expr.size(),
                                 "<quickjs-test>", JS_EVAL_TYPE_GLOBAL);
  if(JS_IsException(result))
    {
      JS_FreeValue(context(), result);
      BOOST_FAIL("JavaScript exception: " << GetExceptionMessage());
    }

  int result_int = 0;
  if(JS_ToInt32(context(), &result_int, result) < 0)
    {
      JS_FreeValue(context(), result);
      BOOST_FAIL("JS_ToInt32 failed, " << GetExceptionMessage());
    }

  BOOST_TEST(result_int == 10);
  JS_FreeValue(context(), result);
}

BOOST_AUTO_TEST_CASE(eval_boolean_expression_should_return_true)
{
  const std::string expr = "\"hello\".length === 5";
  const JSValue jv = JS_Eval(context(), expr.data(), expr.size(),
                             "<eval-boolean>", JS_EVAL_TYPE_GLOBAL);
  if(JS_IsException(jv))
    {
      JS_FreeValue(context(), jv);
      BOOST_FAIL("JavaScript exception: " << GetExceptionMessage());
    }
  auto bool_result = JS_ToBool(context(), jv);
  if(bool_result < 0)
    {
      JS_FreeValue(context(), jv);
      BOOST_FAIL("JS_ToBool failed" << GetExceptionMessage());
    }

  // 1 mean true
  BOOST_TEST(bool_result == 1);
  JS_FreeValue(context(), jv);
}

BOOST_AUTO_TEST_CASE(eval_boolean_expression_should_return_false)
{
  const std::string expr = "\"hello\".length === 4";
  const JSValue jv = JS_Eval(context(), expr.data(), expr.size(),
                             "<eval-boolean>", JS_EVAL_TYPE_GLOBAL);
  if(JS_IsException(jv))
    {
      JS_FreeValue(context(), jv);
      BOOST_FAIL("JavaScript exception: " << GetExceptionMessage());
    }
  auto bool_result = JS_ToBool(context(), jv);
  if(bool_result < 0)
    {
      JS_FreeValue(context(), jv);
      BOOST_FAIL("JS_ToBool failed" << GetExceptionMessage());
    }

  // 0 means false
  BOOST_TEST(bool_result == 0);
  JS_FreeValue(context(), jv);
}

BOOST_AUTO_TEST_CASE(injected_object_property_should_match)
{
  InjectContext();
  BOOST_TEST(
    EvalBooleanExpression(
      "ctx.request.host === 'example.com' && ctx.request.method === 'GET'")
    == true);
}

BOOST_AUTO_TEST_CASE(injected_object_property_should_not_match)
{
  InjectContext();
  BOOST_TEST(EvalBooleanExpression("ctx.request.host === 'other.com'")
             == false);
}

BOOST_AUTO_TEST_CASE(access_unexisted_property_should_return_undefined)
{
  InjectContext();
  BOOST_TEST(EvalBooleanExpression("ctx.request.foo === undefined") == true);
}

BOOST_AUTO_TEST_CASE(access_unexisted_property_should_return_false)
{
  InjectContext();
  BOOST_TEST(EvalBooleanExpression("ctx.request.foo === 1") == false);
}

BOOST_AUTO_TEST_CASE(access_property_of_undefined_should_throw)
{
  InjectContext();
  // `foo` is unexisted, so foo.bar will throw an exception
  BOOST_CHECK_THROW(EvalBooleanExpression("ctx.request.foo.bar === 1"),
                    std::runtime_error);
}

BOOST_AUTO_TEST_SUITE_END()