/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once
#include "proxypp/result.h"
#include "proxypp/script/qjs/value.h"
#include <quickjs.h>

namespace proxypp::script::qjs::detail
{
  class ValueAccess final
  {
  public:
    static Result<Value> Adopt(JSContext& context, JSValue value);
  };

  inline Result<Value> AdoptValue(JSContext& context, JSValue value)
  {
    return ValueAccess::Adopt(context, value);
  }

}