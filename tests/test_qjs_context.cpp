/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#define BOOST_TEST_MODULE test_qjs_context

#include "proxypp/script/qjs/context.h"
#include "proxypp/script/qjs/runtime.h"
#include <boost/test/unit_test.hpp>
#include <type_traits>
#include <utility>

namespace proxypp::script::qjs::test
{
  namespace
  {
    static_assert(!std::is_copy_constructible_v<Context>);
    static_assert(!std::is_copy_assignable_v<Context>);
    static_assert(std::is_move_constructible_v<Context>);
    static_assert(std::is_move_assignable_v<Context>);
    static_assert(std::is_nothrow_move_constructible_v<Context>);
    static_assert(std::is_nothrow_move_assignable_v<Context>);

    Runtime MakeRuntime()
    {
      auto runtime = Runtime::Create();
      BOOST_REQUIRE(runtime.has_value());
      BOOST_REQUIRE(runtime->NativeHandle() != nullptr);
      return std::move(*runtime);
    }

    Context MakeContext(Runtime& runtime)
    {
      auto context = Context::Create(runtime);
      BOOST_REQUIRE(context.has_value());
      BOOST_REQUIRE(context->NativeHandle() != nullptr);
      return std::move(*context);
    }
  }

  BOOST_AUTO_TEST_SUITE(qjs_context_tests)

  BOOST_AUTO_TEST_CASE(context_create_should_return_valid_context)
  {
    auto runtime = MakeRuntime();
    auto context = Context::Create(runtime);
    BOOST_TEST(context.has_value());
    BOOST_TEST(context->NativeHandle() != nullptr);
  }

  BOOST_AUTO_TEST_CASE(context_should_be_move_only)
  {
    BOOST_TEST(!std::is_copy_constructible_v<Context>);
    BOOST_TEST(!std::is_copy_assignable_v<Context>);
    BOOST_TEST(std::is_move_constructible_v<Context>);
    BOOST_TEST(std::is_move_assignable_v<Context>);
    BOOST_TEST(std::is_nothrow_move_constructible_v<Context>);
    BOOST_TEST(std::is_nothrow_move_assignable_v<Context>);
  }

  BOOST_AUTO_TEST_CASE(context_move_construct_should_transfer_context_ownership)
  {
    auto runtime = MakeRuntime();
    auto context = MakeContext(runtime);
    auto* native_handle = context.NativeHandle();
    Context moved { std::move(context) };
    BOOST_TEST(moved.NativeHandle() == native_handle);
    BOOST_TEST(moved.NativeHandle() != nullptr);
    // after being moved from, the old object's NativeHandle should be nullptr
    BOOST_TEST(context.NativeHandle() == nullptr);
  }

  BOOST_AUTO_TEST_CASE(
    context_move_assign_should_release_old_context_and_take_new_context)
  {
    auto runtime = MakeRuntime();
    auto lhs_ctx = MakeContext(runtime);
    auto rhs_ctx = MakeContext(runtime);
    auto* lhs_old_native_handle = lhs_ctx.NativeHandle();
    auto* rhs_native_handle = rhs_ctx.NativeHandle();

    lhs_ctx = std::move(rhs_ctx);

    BOOST_TEST(lhs_ctx.NativeHandle() == rhs_native_handle);
    BOOST_TEST(lhs_ctx.NativeHandle() != nullptr);
    BOOST_TEST(lhs_ctx.NativeHandle() != lhs_old_native_handle);
    BOOST_TEST(rhs_ctx.NativeHandle() == nullptr);
  }

  BOOST_AUTO_TEST_CASE(context_self_move_assign_should_keep_context_valid)
  {
    auto runtime = MakeRuntime();
    auto context = MakeContext(runtime);
    auto* native_handle = context.NativeHandle();

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-move"
#endif

    context = std::move(context);

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

    BOOST_TEST(context.NativeHandle() == native_handle);
    BOOST_TEST(context.NativeHandle() != nullptr);
  }

  BOOST_AUTO_TEST_SUITE_END()

}