/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "proxypp/script/qjs/evaluator.h"
#include "proxypp/script/qjs/context.h"
#include "proxypp/script/qjs/detail/exception_message.h"
#include "proxypp/script/qjs/detail/value_access.h"
#include "proxypp/script/qjs/error.h"
#include <format>
#include <quickjs.h>

proxypp::Result<proxypp::script::qjs::Value>
proxypp::script::qjs::Evaluator::Eval(Context& context, std::string_view expr,
                                      std::string_view file_name)
{
  JSContext* qjs_ctx = context.NativeHandle();
  if(qjs_ctx == nullptr)
    {
      return Unexpected(Error { Errc::InvalidArgument });
    }

  const std::string file_name_str { file_name };
  const std::string expr_str { expr };
  JSValue result = JS_Eval(qjs_ctx, expr_str.c_str(), expr_str.size(),
                           file_name_str.c_str(), JS_EVAL_TYPE_GLOBAL);
  if(JS_IsException(result))
    {
      const std::string message = detail::GetExceptionMessage(*qjs_ctx);
      JS_FreeValue(qjs_ctx, result);
      return Unexpected(
        Error { Errc::EvalFailed,
                std::format("{}, from {}", message, file_name_str) });
    }

  return detail::AdoptValue(*qjs_ctx, result);
}