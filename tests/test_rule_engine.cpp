/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#define BOOST_TEST_MODULE test_rule_engine

#include "proxypp/rule/error.h"
#include "proxypp/rule/match.h"
#include "proxypp/rule/rule_engine.h"
#include <boost/test/unit_test.hpp>

namespace proxypp::rule::test
{
  struct TestRule
  {
    bool enabled = true;
    std::optional<rule::Match> match;
    std::vector<int> actions;
  };

  struct TestState
  {
    int inject_cnt = 0;
    std::vector<int> executed_actions;
  };

  rule::RuleEngine CreateEngine()
  {
    auto engine = RuleEngine::Create();
    BOOST_REQUIRE(engine.has_value());
    return std::move(*engine);
  }

  auto MakeFilter()
  {
    return [](const TestRule& rule) { return rule.enabled; };
  }

  auto MakeInjector(TestState& state)
  {
    return [&state](MatchContext&) -> Result<void> {
      ++state.inject_cnt;
      return {};
    };
  }

  auto MakeMatchGetter()
  {
    return [](const TestRule& rule) -> const std::optional<Match>& {
      return rule.match;
    };
  }

  auto MakeActionsGetter()
  {
    return [](const TestRule& rule) -> const std::vector<int>& {
      return rule.actions;
    };
  }

  auto MakeActionExecutor(TestState& state)
  {
    return [&state](int action) -> Result<void> {
      state.executed_actions.push_back(action);
      return {};
    };
  }

  BOOST_AUTO_TEST_CASE(apply_rules_should_execute_unconditional_rule_actions)
  {
    auto engine = CreateEngine();
    TestState state;
    const std::vector<TestRule> rules { TestRule {
      .enabled = true,
      .match = std::nullopt,
      .actions = std::vector<int>({ 1, 2, 3 }) } };
    const auto result = engine.ApplyRules(
      rules, MakeFilter(), MakeInjector(state), MakeMatchGetter(),
      MakeActionsGetter(), MakeActionExecutor(state));
    BOOST_REQUIRE(result.has_value());
    BOOST_TEST(state.executed_actions == std::vector({ 1, 2, 3 }),
               boost::test_tools::per_element());
  }

  BOOST_AUTO_TEST_CASE(apply_rules_should_skip_filtered_rules)
  {
    auto engine = CreateEngine();
    TestState state;
    const std::vector<TestRule> rules {
      TestRule { .enabled = false,
                 .match = std::nullopt,
                 .actions = std::vector<int>({ 1 }) },
      TestRule { .enabled = true,
                 .match = std::nullopt,
                 .actions = std::vector<int>({ 2 }) },
    };
    const auto result = engine.ApplyRules(
      rules, MakeFilter(), MakeInjector(state), MakeMatchGetter(),
      MakeActionsGetter(), MakeActionExecutor(state));
    BOOST_REQUIRE(result.has_value());
    BOOST_TEST(state.executed_actions == std::vector<int>({ 2 }),
               boost::test_tools::per_element());
  }

  BOOST_AUTO_TEST_CASE(
    apply_rules_should_not_inject_context_for_unconditional_rules)
  {
    auto engine = CreateEngine();

    TestState state;

    const std::vector<TestRule> rules {
      TestRule { .enabled = true,
                 .match = std::nullopt,
                 .actions = std::vector<int>({ 1 }) },
    };

    const auto result = engine.ApplyRules(
      rules, MakeFilter(), MakeInjector(state), MakeMatchGetter(),
      MakeActionsGetter(), MakeActionExecutor(state));
    BOOST_REQUIRE(result.has_value());
    BOOST_TEST(state.inject_cnt == 0);
    BOOST_TEST(state.executed_actions == std::vector<int>({ 1 }),
               boost::test_tools::per_element());
  }

  BOOST_AUTO_TEST_CASE(apply_rules_should_execute_only_matched_rule_actions)
  {
    auto engine = CreateEngine();
    TestState state;
    const std::vector<TestRule> rules {
      TestRule { .enabled = true,
                 .match = Match { .expr = "1 + 1 === 3" },
                 .actions = std::vector<int>({ 1 }) },
      TestRule { .enabled = true,
                 .match = Match { .expr = "\"hello\".length === 5" },
                 .actions = std::vector<int>({ 2, 3 }) },
    };
    const auto result = engine.ApplyRules(
      rules, MakeFilter(), MakeInjector(state), MakeMatchGetter(),
      MakeActionsGetter(), MakeActionExecutor(state));
    BOOST_REQUIRE(result.has_value());
    BOOST_TEST(state.inject_cnt == 2);
    BOOST_TEST(state.executed_actions == std::vector<int>({ 2, 3 }),
               boost::test_tools::per_element());
  }

  BOOST_AUTO_TEST_CASE(apply_rules_should_skip_actions_when_match_is_false)
  {
    auto engine = CreateEngine();
    TestState state;
    const std::vector<TestRule> rules {
      TestRule { .enabled = true,
                 .match = Match { .expr = "1 + 1 === 3" },
                 .actions = std::vector<int>({ 1, 2, 3 }) },
    };
    const auto result = engine.ApplyRules(
      rules, MakeFilter(), MakeInjector(state), MakeMatchGetter(),
      MakeActionsGetter(), MakeActionExecutor(state));
    BOOST_REQUIRE(result.has_value());
    BOOST_TEST(state.inject_cnt == 1);
    BOOST_TEST(state.executed_actions.empty());
  }

  BOOST_AUTO_TEST_CASE(
    apply_rules_should_return_error_when_match_result_is_not_boolean)
  {
    auto engine = CreateEngine();
    TestState state;
    const std::vector<TestRule> rules {
      TestRule { .enabled = true,
                 .match = Match { .expr = "\"foo\"" },
                 .actions = std::vector<int>({ 1, 2, 3 }) },
    };
    const auto result = engine.ApplyRules(
      rules, MakeFilter(), MakeInjector(state), MakeMatchGetter(),
      MakeActionsGetter(), MakeActionExecutor(state));
    BOOST_REQUIRE(!result.has_value());
    BOOST_TEST(result.error().code() == Errc::MatchResultNotBoolean);
    BOOST_TEST(state.executed_actions.empty());
  }

  BOOST_AUTO_TEST_CASE(
    apply_rules_should_return_error_when_match_expression_is_invalid)
  {
    auto engine = CreateEngine();
    TestState state;
    const std::vector<TestRule> rules {
      TestRule { .enabled = true,
                 .match = Match { .expr = "\"foo" },
                 .actions = std::vector<int>({ 1, 2, 3 }) },
    };
    const auto result = engine.ApplyRules(
      rules, MakeFilter(), MakeInjector(state), MakeMatchGetter(),
      MakeActionsGetter(), MakeActionExecutor(state));
    BOOST_REQUIRE(!result.has_value());
    BOOST_TEST(result.error().code() == Errc::MatchEvaluationFailed);
    BOOST_TEST(state.executed_actions.empty());
  }

  BOOST_AUTO_TEST_CASE(apply_rules_should_stop_when_action_execution_failed)
  {
    auto engine = CreateEngine();
    TestState state;
    const std::vector<TestRule> rules {
      TestRule { .enabled = true,
                 .match = std::nullopt,
                 .actions = std::vector<int>({ 1, 2, 3 }) },
    };

    auto execute_action = [&state](int action) -> Result<void> {
      state.executed_actions.push_back(action);
      if(action == 2)
        {
          return Unexpected(
            Error { Errc::ActionExecutionFailed, "test action failed" });
        }
      return {};
    };

    const auto result = engine.ApplyRules(
      rules, MakeFilter(), MakeInjector(state), MakeMatchGetter(),
      MakeActionsGetter(), execute_action);
    BOOST_REQUIRE(!result.has_value());
    BOOST_TEST(result.error().code() == Errc::ActionExecutionFailed);
    BOOST_TEST(state.executed_actions == std::vector<int>({ 1, 2 }),
               boost::test_tools::per_element());
  }

  BOOST_AUTO_TEST_CASE(
    apply_rules_should_return_error_when_context_injection_failed)
  {
    auto engine = CreateEngine();
    TestState state;
    const std::vector<TestRule> rules {
      TestRule { .enabled = true,
                 .match = Match { .expr = "true" },
                 .actions = std::vector<int>({ 1, 2, 3 }) },
    };

    auto inject_context = [](MatchContext&) -> Result<void> {
      return Unexpected(Error { Errc::RuleContextPreparationFailed });
    };

    const auto result = engine.ApplyRules(
      rules, MakeFilter(), inject_context, MakeMatchGetter(),
      MakeActionsGetter(), MakeActionExecutor(state));
    BOOST_REQUIRE(!result.has_value());
    BOOST_TEST(result.error().code() == Errc::RuleContextPreparationFailed);
    BOOST_TEST(state.executed_actions.empty());
  }

}