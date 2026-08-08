/*
 * Copyright 2026-present The proxy++ authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "log.h"
#include <array>
#include <filesystem>
#include <magic_enum/magic_enum.hpp>
#include <mutex>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace proxypp::log
{
  namespace
  {
    constexpr auto kConsolePattern = "%H:%M:%S.%e %8t %^[%-4n] [%-6l] %v%$";
    constexpr auto kFilePattern
      = "[%Y-%m-%d %H:%M:%S.%e] [%8t] [%-4n] [%-6l] %v";

    constexpr auto kModuleCount = magic_enum::enum_count<Module>();

    using LoggerArray
      = std::array<std::shared_ptr<spdlog::logger>, kModuleCount>;

    std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> MakeConsoleSink()
    {
      auto console_sink
        = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
      console_sink->set_pattern(kConsolePattern);
      return console_sink;
    }

    std::shared_ptr<spdlog::sinks::rotating_file_sink_mt>
    MakeRotateFileSink(const std::filesystem::path& log_dir, int max_file_size,
                       int max_file_count)
    {
      const auto file_sink
        = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
          (log_dir / "proxypp.log").string(), max_file_size, max_file_count);
      file_sink->set_pattern(kFilePattern);
      return file_sink;
    }

    LoggerArray MakeLoggers()
    {
      const auto console_sink = MakeConsoleSink();
      LoggerArray result {};
      for(const auto module : magic_enum::enum_values<Module>())
        {
          const auto index = magic_enum::enum_index(module);
          const auto name = std::string { magic_enum::enum_name(module) };
          auto logger = std::make_shared<spdlog::logger>(name, console_sink);
          logger->set_level(spdlog::level::info);
          logger->flush_on(spdlog::level::warn);
          spdlog::register_logger(logger);
          result[*index] = std::move(logger);
        }
      return result;
    }

    LoggerArray& Loggers()
    {
      static auto loggers = MakeLoggers();
      return loggers;
    }
  }

  void Init(const std::filesystem::path& log_dir)
  {
    static std::once_flag init_flag;
    std::call_once(init_flag, [&] {
      constexpr auto kMaxFileSize = 10 * 1024 * 1024;
      constexpr auto kMaxFileCount = 5;
      for(const auto& logger : Loggers())
        {
          const auto file_sink
            = MakeRotateFileSink(log_dir, kMaxFileSize, kMaxFileCount);
          logger->sinks().push_back(file_sink);
        }
    });
  }

  std::shared_ptr<spdlog::logger> Get(Module module)
  {
    const auto index = magic_enum::enum_index(module);
    return Loggers()[*index];
  }

  void SetLevel(Module module, spdlog::level::level_enum level)
  {
    Get(module)->set_level(level);
  }

  void SetAllLevels(spdlog::level::level_enum level)
  {
    for(const auto& logger : Loggers())
      {
        logger->set_level(level);
      }
  }

}
