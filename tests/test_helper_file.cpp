/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#define BOOST_TEST_MODULE file_helper_tests

#include "proxypp/error.h"
#include "proxypp/helper/file_helper.h"
#include "proxypp/result.h"
#include <boost/test/unit_test.hpp>
#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <string_view>

namespace proxypp::helper::file::test
{

  Result<std::filesystem::path> CreateTempFile(std::string_view content)
  {
    namespace fs = std::filesystem;
    const auto file_name = std::format(
      "proxypp_test_{}.txt",
      std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count()));

    const auto temp_file_path = fs::temp_directory_path() / file_name;
    if(fs::exists(temp_file_path) && fs::is_regular_file(temp_file_path))
      {
        fs::remove(temp_file_path);
      }

    // Write in binary mode so '\n' is preserved instead of being converted to
    // "\r\n" on Windows.
    std::ofstream file(temp_file_path.string(),
                       std::ios::binary | std::ios::trunc);
    if(!file)
      {
        return Unexpected(Error(Errc::InternalError));
      }

    file.write(content.data(), content.size());
    file.close();

    if(!file)
      {
        return Unexpected(Error(Errc::InternalError));
      }

    return temp_file_path;
  }

  std::string DumpBytes(std::string_view text)
  {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');

    for(unsigned char ch : text)
      {
        oss << std::setw(2) << static_cast<int>(ch) << ' ';
      }

    return oss.str();
  }

  BOOST_AUTO_TEST_CASE(read_text_file_should_return_content_when_file_exists)
  {
    std::string_view content = "let do it!\n\n";
    auto temp_file_result = CreateTempFile(content);
    BOOST_REQUIRE(temp_file_result.has_value());
    BOOST_REQUIRE(std::filesystem::exists(*temp_file_result));

    auto read_result = file::ReadTextFile(*temp_file_result);
    BOOST_REQUIRE(read_result.has_value());
    BOOST_TEST((*read_result == content));

    BOOST_TEST_MESSAGE("expected size: " << content.size());
    BOOST_TEST_MESSAGE("actual size: " << read_result->size());
    BOOST_TEST_MESSAGE("expected bytes: " << DumpBytes(content));
    BOOST_TEST_MESSAGE("actual bytes: " << DumpBytes(*read_result));
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
    std::string_view content = "";
    auto temp_file_result = CreateTempFile(content);
    BOOST_REQUIRE(temp_file_result.has_value());
    BOOST_REQUIRE(std::filesystem::exists(*temp_file_result));

    auto read_result = file::ReadTextFile(*temp_file_result);
    BOOST_REQUIRE(read_result.has_value());
    BOOST_TEST(*read_result == content);
  }
}