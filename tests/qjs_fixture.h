/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "proxypp/script/qjs.h"
#include <boost/test/unit_test.hpp>

namespace proxypp::script::qjs::test
{
  Runtime MakeRuntime();

  Context MakeContext(Runtime& runtime);

  struct QjsContextFixture
  {
    QjsContextFixture() : runtime(MakeRuntime()), context(MakeContext(runtime))
    {}
    Runtime runtime;
    Context context;
  };

  template <typename T>
  void RequireErrorCode(const Result<T>& result, Errc errc)
  {
    BOOST_REQUIRE(!result.has_value());
    BOOST_TEST(result.error().code == errc);
  }

  template <typename T>
  void RequireErrorMessageContains(const Result<T>& result,
                                   std::string_view expected)
  {
    BOOST_REQUIRE(!result.has_value());
    const auto& message = result.error().message;
    BOOST_TEST_INFO("actual message: " << message);
    BOOST_TEST(result.error().message.find(expected) != std::string::npos);
  }
}