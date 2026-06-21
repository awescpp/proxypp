/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#define BOOST_TEST_MODULE test_qjs_runtime

#include "proxypp/script/qjs/runtime.h"
#include <boost/test/unit_test.hpp>
#include <type_traits>
#include <utility>

namespace proxypp::script::qjs::test
{
  namespace
  {
    static_assert(!std::is_copy_constructible_v<Runtime>);
    static_assert(!std::is_copy_assignable_v<Runtime>);
    static_assert(std::is_move_constructible_v<Runtime>);
    static_assert(std::is_move_assignable_v<Runtime>);
    static_assert(std::is_nothrow_move_constructible_v<Runtime>);
    static_assert(std::is_nothrow_move_assignable_v<Runtime>);

    Runtime MakeRuntime()
    {
      auto runtime = Runtime::Create();
      BOOST_REQUIRE(runtime.has_value());
      BOOST_REQUIRE(runtime->NativeHandle() != nullptr);
      // Runtime is non-copyable and can only be moved
      return std::move(*runtime);
    }
  }

  BOOST_AUTO_TEST_SUITE(qjs_runtime_tests)

  BOOST_AUTO_TEST_CASE(runtime_create_should_return_valid_runtime)
  {
    auto runtime = Runtime::Create();
    BOOST_REQUIRE(runtime.has_value());
    BOOST_TEST(runtime->NativeHandle() != nullptr);
  }

  BOOST_AUTO_TEST_CASE(runtime_should_be_move_only)
  {
    BOOST_TEST(!std::is_copy_constructible_v<Runtime>);
    BOOST_TEST(!std::is_copy_assignable_v<Runtime>);
    BOOST_TEST(std::is_move_constructible_v<Runtime>);
    BOOST_TEST(std::is_move_assignable_v<Runtime>);
    BOOST_TEST(std::is_nothrow_move_constructible_v<Runtime>);
    BOOST_TEST(std::is_nothrow_move_assignable_v<Runtime>);
  }

  BOOST_AUTO_TEST_CASE(runtime_move_construct_should_transfer_runtime_ownership)
  {
    auto runtime = MakeRuntime();
    auto* native_handle = runtime.NativeHandle();
    Runtime moved { std::move(runtime) };
    BOOST_TEST(moved.NativeHandle() == native_handle);
    BOOST_TEST(moved.NativeHandle() != nullptr);
    BOOST_TEST(runtime.NativeHandle() == nullptr);
  }

  BOOST_AUTO_TEST_CASE(
    runtime_move_assign_should_release_old_runtime_and_take_new_runtime)
  {
    auto lhs = MakeRuntime();
    auto rhs = MakeRuntime();
    auto* old_native_handle = lhs.NativeHandle();
    auto* rhs_native_handle = rhs.NativeHandle();
    lhs = std::move(rhs);
    BOOST_TEST(lhs.NativeHandle() == rhs_native_handle);
    BOOST_TEST(lhs.NativeHandle() != nullptr);
    BOOST_TEST(lhs.NativeHandle() != old_native_handle);
    BOOST_TEST(rhs.NativeHandle() == nullptr);
  }

  BOOST_AUTO_TEST_CASE(runtime_self_assign_should_keep_runtime_valid)
  {
    auto runtime = MakeRuntime();
    auto native_handle = runtime.NativeHandle();
    runtime = std::move(runtime);
    BOOST_TEST(runtime.NativeHandle() == native_handle);
    BOOST_TEST(runtime.NativeHandle() != nullptr);
  }

  BOOST_AUTO_TEST_SUITE_END()

}