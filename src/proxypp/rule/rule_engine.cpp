/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "proxypp/rule/rule_engine.h"
#include "proxypp/rule/error.h"
#include "proxypp/script/qjs.h"

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
        Error error;
        error.code = Errc::RuntimeCreationFailed;
        error.message = std::format("create qjs runtime failed: {}",
                                    runtime.error().message);
        return Unexpected(error);
      }
    auto impl = std::make_unique<Impl>(std::move(*runtime));
    return RuleEngine { std::move(impl) };
  }

  Result<MatchContext> RuleEngine::CreateMatchContext() const
  {
    auto context = script::qjs::Context::Create(impl_->runtime_);
    if(!context)
      {
        Error error;
        error.code = Errc::ContextCreationFailed;
        error.message = std::format("create match context failed: {}",
                                    context.error().message);
        return Unexpected(error);
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
        Error error;
        error.code = Errc::MatchEvaluationFailed;
        error.message = std::format("evaluate \"{}\" failed: {}", expr,
                                    result.error().message);
        return Unexpected(error);
      }

    if(!result->IsBool())
      {
        Error error;
        error.code = Errc::MatchResultNotBoolean;
        error.message = std::format(
          "evaluate \"{}\" failed, result is not a bool value", expr);
        return Unexpected(error);
      }
    return result->ToBool();
  }

}