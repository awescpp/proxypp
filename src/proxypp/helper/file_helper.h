/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "proxypp/helper/file_helper.h"
#include "proxypp/result.h"
#include <filesystem>

namespace proxypp::helper::file
{
  Result<std::string> ReadTextFile(const std::filesystem::path& path);
}