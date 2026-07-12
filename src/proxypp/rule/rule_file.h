/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "proxypp/result.h"
#include "proxypp/rule/config.h"
#include <filesystem>

namespace proxypp::rule
{
  Result<rule::Config>
  LoadRulesFromFile(const std::filesystem::path& file_path);
}