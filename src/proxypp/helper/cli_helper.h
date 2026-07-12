/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <CLI/CLI.hpp>
#include <filesystem>

namespace proxypp::helper::cli
{
  std::filesystem::path
  ResolveRuleFilePath(const CLI::Option& rule_file_option,
                      std::string_view parsed_rule_file);
}