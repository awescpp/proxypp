/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "qjs_fixture.h"

proxypp::script::qjs::Runtime proxypp::script::qjs::test::MakeRuntime()
{
  auto runtime = qjs::Runtime::Create();
  BOOST_REQUIRE(runtime.has_value());
  BOOST_REQUIRE(runtime->NativeHandle() != nullptr);
  return std::move(*runtime);
}

proxypp::script::qjs::Context
proxypp::script::qjs::test::MakeContext(Runtime& runtime)
{
  auto context = qjs::Context::Create(runtime);
  BOOST_REQUIRE(context.has_value());
  BOOST_REQUIRE(context->NativeHandle() != nullptr);
  return std::move(*context);
}