/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <boost/test/unit_test.hpp>

namespace proxypp::script::qjs::test
{
  inline qjs::Runtime CreateRuntime()
  {
    auto runtime = qjs::Runtime::Create();
    BOOST_REQUIRE(runtime.has_value());
    return std::move(*runtime);
  }

  inline rule::MatchContext CreateMatchContext(qjs::Runtime& runtime)
  {
    auto context = qjs::Context::Create(runtime);
    BOOST_REQUIRE(context.has_value());
    auto result = rule::MatchContext::Create(std::move(*context));
    BOOST_REQUIRE(result.has_value());
    return std::move(*result);
  }

  inline std::string RequireString(const Result<qjs::Value>& result)
  {
    BOOST_REQUIRE(result.has_value());
    BOOST_REQUIRE(result->IsValid());
    BOOST_REQUIRE(result->IsString());
    auto text = result->ToString();
    BOOST_REQUIRE(text.has_value());
    return std::move(*text);
  }

  inline int RequireInt32(const Result<qjs::Value>& result)
  {
    BOOST_REQUIRE(result.has_value());
    BOOST_REQUIRE(result->IsValid());
    BOOST_REQUIRE(result->IsNumber());
    auto number = result->ToInt32();
    BOOST_REQUIRE(number.has_value());
    return *number;
  }
}