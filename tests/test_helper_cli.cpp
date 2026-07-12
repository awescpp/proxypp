/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#define BOOST_TEST_MODULE test_cli_helpers

#include "proxypp/helper/cli_helper.h"
#include <CLI/CLI.hpp>
#include <boost/dll.hpp>
#include <boost/test/unit_test.hpp>
#include <filesystem>
#include <string>

namespace proxypp::helper::cli::test
{
  BOOST_AUTO_TEST_CASE(
    resolve_rule_file_should_return_user_provided_absolute_path_when_option_is_present)
  {
    using namespace std::filesystem;
    CLI::App app;
    std::string parsed_rule_file;
    auto* rule_file_opt = app.add_option("-r,--rule-file", parsed_rule_file);
    const char* argv[] = { "proxy++", "--rule-file", "config/rules.json" };
    app.parse(std::size(argv), argv);
    auto rule_file_path
      = ResolveRuleFilePath(*rule_file_opt, parsed_rule_file);

    BOOST_TEST(rule_file_path == absolute("config/rules.json"));
    BOOST_TEST_MESSAGE("rule_file_path: " << rule_file_path.string());
  }

  BOOST_AUTO_TEST_CASE(
    resolve_file_path_should_return_default_rules_json_when_option_is_absent)
  {
    using namespace std::filesystem;
    CLI::App app;
    std::string parsed_rule_file;
    auto* rule_file_opt = app.add_option("-r,--rule-file", parsed_rule_file);
    const char* argv[] = { "proxy++" };
    app.parse(std::size(argv), argv);
    auto rule_file_path
      = ResolveRuleFilePath(*rule_file_opt, parsed_rule_file);

    auto installed_dir
      = path { boost::dll::program_location().parent_path().string() };
    BOOST_TEST(rule_file_path == (installed_dir / "rules.json"));
    BOOST_TEST_MESSAGE("rule_file_path: " << rule_file_path.string());
  }
}