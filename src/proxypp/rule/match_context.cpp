/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "proxypp/rule/match_context.h"
#include "proxypp/rule/error.h"
#include "proxypp/script/qjs/value.h"

namespace proxypp::rule
{
  class MatchContext::Impl
  {
  public:
    explicit Impl(script::qjs::Context context) : context_(std::move(context))
    {}

    script::qjs::Context context_;
  };

  MatchContext::MatchContext(std::unique_ptr<Impl> impl)
      : impl_(std::move(impl))
  {}

  Result<MatchContext> MatchContext::Create(script::qjs::Context context)
  {
    auto impl = std::make_unique<Impl>(std::move(context));
    return MatchContext { std::move(impl) };
  }

  MatchContext::~MatchContext() = default;

  MatchContext::MatchContext(MatchContext&& other) noexcept = default;

  MatchContext&
  MatchContext::operator=(MatchContext&& other) noexcept = default;

  script::qjs::Context& MatchContext::ScriptContext() noexcept
  {
    return impl_->context_;
  }

  Result<void>
  MatchContext::AddObject(std::string_view name, script::qjs::Value object)
  {
    if(!object.IsValid() || !object.IsObject())
      {
        return Unexpected(
          Error(Errc::RuleContextPreparationFailed, "value is not an object"));
      }
    return AddGlobalValue(name, std::move(object));
  }

  Result<void>
  MatchContext::AddGlobalValue(std::string_view name, script::qjs::Value value)
  {
    auto global = script::qjs::Value::GlobalObject(impl_->context_);
    if(!global.has_value())
      {
        return Unexpected(
          Error(Errc::RuleContextPreparationFailed, global.error().message()));
      }

    if(auto result = global->SetProperty(name, std::move(value));
       !result.has_value())
      {
        return Unexpected(
          Error(Errc::RuleContextPreparationFailed, result.error().message()));
      }

    return {};
  }

}