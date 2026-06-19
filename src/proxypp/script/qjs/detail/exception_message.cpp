/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "proxypp/script/qjs/detail/exception_message.h"
#include <quickjs.h>

std::string
proxypp::script::qjs::detail::GetExceptionMessage(JSContext& context)
{
  JSValue exception = JS_GetException(&context);

  if(JS_IsNull(exception) || JS_IsUndefined(exception))
    {
      JS_FreeValue(&context, exception);
      return "<unknown QuickJS exception>";
    }

  const char* c_str = JS_ToCString(&context, exception);
  if(c_str == nullptr)
    {
      JSValue convert_exception = JS_GetException(&context);
      JS_FreeValue(&context, convert_exception); // drain the exception

      JS_FreeValue(&context, exception);
      return "<failed to convert QuickJS exception to string>";
    }

  std::string message{c_str};

  JS_FreeCString(&context, c_str);
  JS_FreeValue(&context, exception);

  return message;
}