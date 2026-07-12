/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "proxypp/error.h"
#include "proxypp/result.h"
#include <filesystem>
#include <format>
#include <fstream>
#include <sstream>

namespace proxypp::test::helper
{
  inline Result<std::filesystem::path>
  CreateTempFile(std::string_view content, std::string_view extension = "txt")
  {
    namespace fs = std::filesystem;
    using clock = std::chrono::steady_clock;
    const auto file_name = std::format(
      "proxypp_test_{}.{}",
      std::to_string(clock::now().time_since_epoch().count()), extension);

    auto temp_file_path = fs::temp_directory_path() / file_name;
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

  inline std::string DumpBytes(std::string_view text)
  {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');

    for(const unsigned char ch : text)
      {
        oss << std::setw(2) << static_cast<int>(ch) << ' ';
      }

    return oss.str();
  }

}