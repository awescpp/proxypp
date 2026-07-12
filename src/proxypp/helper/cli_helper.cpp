/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "proxypp/helper/cli_helper.h"
#include <boost/dll.hpp>

namespace proxypp::helper::cli
{
  std::filesystem::path
  ResolveRuleFilePath(const CLI::Option& rule_file_option,
                      std::string_view parsed_rule_file)
  {
    using namespace std::filesystem;
    // if `--rule-file` presents
    if(rule_file_option.count() > 0)
      {
        // User-provided rule file has already been validated by CLI11.
        return absolute(path { parsed_rule_file });
      }
    auto default_file_name = "rules.json";
    auto exec_dir
      = path { boost::dll::program_location().parent_path().string() };
    return exec_dir / default_file_name;
  }
}