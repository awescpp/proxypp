/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <filesystem>
#include <memory>
#include <spdlog/spdlog.h>

namespace proxypp::log
{
  enum class Module
  {
    app,
    core,
    http,
    rule,
    qjs
  };

  void Init(const std::filesystem::path& log_dir);

  [[nodiscard]]
  std::shared_ptr<spdlog::logger> Get(Module module);

  void SetLevel(Module module, spdlog::level::level_enum level);

  void SetAllLevels(spdlog::level::level_enum level);
}
