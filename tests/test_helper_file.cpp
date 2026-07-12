/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#define BOOST_TEST_MODULE file_helper_tests

#include "proxypp/helper/file_helper.h"
#include "proxypp/result.h"
#include "test_helper.h"
#include <boost/test/unit_test.hpp>
#include <chrono>
#include <filesystem>
#include <string_view>

namespace proxypp::helper::file::test
{

  BOOST_AUTO_TEST_CASE(read_text_file_should_return_content_when_file_exists)
  {
    const std::string_view content = "let do it!\n\n";
    auto temp_file_result = proxypp::test::helper::CreateTempFile(content);
    BOOST_REQUIRE(temp_file_result.has_value());
    BOOST_REQUIRE(std::filesystem::exists(*temp_file_result));

    auto read_result = file::ReadTextFile(*temp_file_result);
    BOOST_REQUIRE(read_result.has_value());
    BOOST_TEST((*read_result == content));

    BOOST_TEST_MESSAGE("expected size: " << content.size());
    BOOST_TEST_MESSAGE("actual size: " << read_result->size());
    BOOST_TEST_MESSAGE(
      "expected bytes: " << proxypp::test::helper::DumpBytes(content));
    BOOST_TEST_MESSAGE(
      "actual bytes: " << proxypp::test::helper::DumpBytes(*read_result));
  }

  BOOST_AUTO_TEST_CASE(read_text_file_should_return_error_when_file_not_exists)
  {
    namespace fs = std::filesystem;
    auto unexisted_file = fs::temp_directory_path() / "a.unexisted.file";
    BOOST_REQUIRE(!fs::exists(unexisted_file));

    auto read_result = file::ReadTextFile(unexisted_file);
    BOOST_REQUIRE(!read_result.has_value());
  }

  BOOST_AUTO_TEST_CASE(
    read_text_file_should_return_empty_string_when_file_is_empty)
  {
    const std::string_view content = "";
    auto temp_file_result = proxypp::test::helper::CreateTempFile(content);
    BOOST_REQUIRE(temp_file_result.has_value());
    BOOST_REQUIRE(std::filesystem::exists(*temp_file_result));

    auto read_result = file::ReadTextFile(*temp_file_result);
    BOOST_REQUIRE(read_result.has_value());
    BOOST_TEST((read_result == content));
  }
}