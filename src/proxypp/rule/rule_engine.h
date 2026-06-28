/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "proxypp/class_utils.h"
#include "proxypp/result.h"
#include "proxypp/rule/match_context.h"
#include <functional>

namespace proxypp::rule
{
  namespace detail
  {
    template <typename Actions, typename ActionExecutor>
    Result<void>
    ExecuteActions(const Actions& actions, ActionExecutor&& execute_action)
    {
      for(const auto& action : actions)
        {
          if(auto result = std::invoke(execute_action, action);
             !result.has_value())
            {
              return result;
            }
        }
      return {};
    }
  }

  class RuleEngine
  {
  public:
    static Result<RuleEngine> Create();

    ~RuleEngine();

    PROXYPP_DISABLE_COPY(RuleEngine);

    RuleEngine(RuleEngine&& other) noexcept;

    RuleEngine& operator=(RuleEngine&& other) noexcept;

    /**
     * @brief Applies a sequence of rules using the common proxy++ rule
     * execution flow.
     *
     * This function implements the protocol-independent part of rule
     * execution:
     *
     *   1. Iterate over rules in their original order.
     *   2. Use @p filter to decide whether the current rule should participate
     * in the current execution phase.
     *   3. Use @p get_match to obtain the optional match expression of the
     * rule.
     *   4. If the rule has no match expression, treat it as matched.
     *   5. If the rule has a match expression, create a MatchContext, call
     *      @p inject_context to populate it, and evaluate the expression.
     *   6. If the match result is true, use @p get_actions to obtain the
     * rule's actions and execute them in order through @p execute_action.
     *
     * RuleEngine only owns and manages the common execution infrastructure,
     * such as creating MatchContext objects and evaluating match expressions.
     * It does not know the concrete rule type, protocol phase, adapter type,
     * or action type. Those details are supplied by the caller through the
     * callback parameters.
     *
     * A rule without a match expression is considered an unconditional rule:
     * its actions are executed as long as @p filter accepts the rule.
     *
     * Match contexts are created only for rules that actually have match
     * expressions. This avoids creating scripting contexts for unconditional
     * rules.
     *
     * Actions are executed in declaration order. The first failed action stops
     * the current ApplyRules call and returns the error to the caller. In
     * other words, this function uses fail-fast semantics for action
     * execution.
     *
     * @tparam Rules
     *   A range-like type whose elements are concrete rule objects. The rule
     * type does not need to inherit from a common abstract base class.
     *
     * @tparam RuleFilter
     *   Callable type invoked as:
     *
     *     bool filter(const Rule& rule);
     *
     *   It should return true if the rule should be considered in the current
     *   execution phase, and false otherwise. For example, HTTP request
     * execution may filter for enabled request-phase rules.
     *
     * @tparam ContextInjector
     *   Callable type invoked as:
     *
     *     Result<void> inject_context(MatchContext& context);
     *
     *   It should populate the provided MatchContext with protocol-specific
     * data, such as request method, request target, request headers, response
     * status, or response headers.
     *
     * @tparam MatchGetter
     *   Callable type invoked as:
     *
     *     const std::optional<Match>& get_match(const Rule& rule);
     *
     *   It should return the optional match expression associated with the
     * rule. If no match is present, the rule is treated as matched
     * unconditionally.
     *
     * @tparam ActionsGetter
     *   Callable type invoked as:
     *
     *     const Actions& get_actions(const Rule& rule);
     *
     *   It should return the ordered action list associated with the rule.
     *
     * @tparam ActionExecutor
     *   Callable type invoked for each action as:
     *
     *     Result<void> execute_action(const Action& action);
     *
     *   It should execute one concrete action against the caller-provided
     * adapter or execution target.
     *
     * @param rules
     *   Ordered rule collection to apply.
     *
     * @param filter
     *   Protocol-specific rule filter.
     *
     * @param inject_context
     *   Protocol-specific match context injector.
     *
     * @param get_match
     *   Callable used to obtain the rule's optional match expression.
     *
     * @param get_actions
     *   Callable used to obtain the rule's action list.
     *
     * @param execute_action
     *   Callable used to execute one action.
     *
     * @return
     *   Result<void> containing success if all applicable rules are processed
     *   successfully. Returns an error if context creation, context injection,
     *   match evaluation, or action execution fails.
     *
     * @note
     *   This function does not own @p rules or any object captured by the
     * provided callables. Callers must ensure that all referenced adapters,
     * configs, and rule objects outlive this call.
     */
    template <typename Rules, typename RuleFilter, typename ContextInjector,
              typename MatchGetter, typename ActionsGetter,
              typename ActionExecutor>
    Result<void>
    ApplyRules(const Rules& rules, RuleFilter&& filter,
               ContextInjector&& inject_context, MatchGetter&& get_match,
               ActionsGetter&& get_actions, ActionExecutor&& execute_action)
    {
      for(const auto& rule : rules)
        {
          if(!std::invoke(filter, rule))
            {
              continue;
            }

          const auto& match = std::invoke(get_match, rule);
          const auto& actions = std::invoke(get_actions, rule);

          if(!match.has_value())
            {
              if(auto result = detail::ExecuteActions(actions, execute_action);
                 !result.has_value())
                {
                  return result;
                }
              continue;
            }

          auto context = CreateMatchContext();
          if(!context.has_value())
            {
              return Unexpected(context.error());
            }

          if(auto result = std::invoke(inject_context, *context);
             !result.has_value())
            {
              return result;
            }

          auto matched = EvaluateMatch(*context, match->expr);
          if(!matched.has_value())
            {
              return Unexpected(matched.error());
            }

          if(!*matched)
            {
              continue;
            }

          if(auto result = detail::ExecuteActions(actions, execute_action);
             !result.has_value())
            {
              return result;
            }
        }

      return {};
    }

  private:
    Result<MatchContext> CreateMatchContext() const;

    Result<bool> EvaluateMatch(MatchContext& context, std::string_view expr);

    class Impl;

    explicit RuleEngine(std::unique_ptr<Impl> impl);

    std::unique_ptr<Impl> impl_;
  };
}