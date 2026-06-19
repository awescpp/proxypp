/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once
#include "proxypp/result.h"
#include "proxypp/script/qjs/value.h"

namespace proxypp::script::qjs
{
  class Context;
  class Evaluator
  {
  public:
    static Result<Value> Eval(Context& context, std::string_view expr,
                              std::string_view file_name = "<proxypp-qjs>");
  };
}