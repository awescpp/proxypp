/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "proxypp/rule/rule_engine.h"
#include "proxypp/log/log.h"
#include "proxypp/rule/error.h"
#include "proxypp/script/qjs.h"
#include <format>

namespace
{
  const auto logger = proxypp::log::Get(proxypp::log::Module::rule);
}

namespace proxypp::rule
{
  class RuleEngine::Impl
  {
  public:
    explicit Impl(script::qjs::Runtime runtime) : runtime_(std::move(runtime))
    {}
    script::qjs::Runtime runtime_;
  };

  RuleEngine::RuleEngine(std::unique_ptr<Impl> impl) : impl_(std::move(impl))
  {}

  RuleEngine::~RuleEngine() = default;

  RuleEngine::RuleEngine(RuleEngine&& other) noexcept = default;

  RuleEngine& RuleEngine::operator=(RuleEngine&& other) noexcept = default;

  Result<RuleEngine> RuleEngine::Create()
  {
    auto runtime = script::qjs::Runtime::Create();
    if(!runtime)
      {
        return Unexpected(Error(Errc::RuleEngineInitializationFailed,
                                std::format("create qjs runtime failed: {}",
                                            runtime.error().message())));
      }
    auto impl = std::make_unique<Impl>(std::move(*runtime));

    logger->debug("RuleEngine created");

    return RuleEngine { std::move(impl) };
  }

  Result<MatchContext> RuleEngine::CreateMatchContext() const
  {
    auto context = script::qjs::Context::Create(impl_->runtime_);
    if(!context)
      {
        return Unexpected(Error(Errc::RuleEngineInitializationFailed,
                                std::format("create match context failed: {}",
                                            context.error().message())));
      }
    return MatchContext::Create(std::move(*context));
  }

  Result<bool>
  RuleEngine::EvaluateMatch(MatchContext& context, std::string_view expr)
  {
    const auto result
      = script::qjs::Evaluator::Eval(context.ScriptContext(), expr);

    if(!result)
      {
        logger->error(
          R"(failed to evaluate match expression: expr="{}", error="{}")",
          expr, result.error().message());

        return Unexpected(Error {
          Errc::MatchEvaluationFailed,
          std::format(
            R"(failed to evaluate match expression: expr="{}", error="{}")",
            expr, result.error().message()) });
      }

    if(!result->IsBool())
      {
        logger->error(R"(match expression result is not boolean: expr="{}")",
                      expr);

        return Unexpected(Error {
          Errc::MatchResultNotBoolean,
          std::format(R"(match expression result is not boolean: expr="{}")",
                      expr) });
      }

    const auto bool_val = result->ToBool();
    if(!bool_val)
      {
        logger->error(
          R"(failed to convert match expression result to boolean: expr="{}", error="{}")",
          expr, bool_val.error().message());

        return Unexpected(Error {
          Errc::MatchEvaluationFailed,
          std::format(
            R"(failed to convert match expression result to boolean: expr="{}", error="{}")",
            expr, bool_val.error().message()) });
      }

    logger->trace(R"(match expression evaluated: expr="{}", result={})", expr,
                  *bool_val);

    return *bool_val;
  }

}
