/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>

struct JSContext;

namespace proxypp::script::qjs::detail
{
  std::string GetExceptionMessage(JSContext& context);
}