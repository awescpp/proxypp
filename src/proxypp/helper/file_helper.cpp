/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "file_helper.h"
#include "proxypp/error.h"
#include <format>
#include <fstream>
#include <sstream>

namespace proxypp::helper::file
{
  Result<std::string> ReadTextFile(const std::filesystem::path& path)
  {
    std::ifstream file { path, std::ios::binary };
    if(!file)
      {
        return Unexpected(
          // TODO: use FileReadFailed
          Error(Errc::InvalidArgument,
                std::format("failed to open file: {}", path.string())));
      }

    std::stringstream buffer;
    buffer << file.rdbuf();

    // TODO: check if(file.bad())

    return buffer.str();
  }
}