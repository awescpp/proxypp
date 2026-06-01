#include "log.h"

#include <magic_enum/magic_enum.hpp>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace proxypp::log
{
  // Anonymous namespace: used only inside log.cpp, hidden from external code,
  // and not visible during linking.
  namespace
  {
    std::vector<spdlog::sink_ptr> &MakeSinks()
    {
      static std::vector<spdlog::sink_ptr> sinks;

      if(sinks.empty())
        {
          auto console_sink
            = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
          console_sink->set_pattern("[%H:%M:%S.%e] [%n] [%^%l%$] [%s:%#] %v");

          const auto log_file = "logs/proxypp.log";
          constexpr auto file_max_size = 1024 * 1024 * 10;
          constexpr auto max_file_num = 5;
          auto file_sink
            = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
              log_file, file_max_size, max_file_num);
          file_sink->set_pattern(
            "[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] [%t] [%s:%#] %v");

          sinks.push_back(console_sink);
          // TODO: we all file_sink just later
          //  sinks.push_back(file_sink);
        }

      return sinks;
    }

    std::shared_ptr<spdlog::logger> MakeLogger(Module module)
    {
      auto sinks = MakeSinks();
      std::unordered_map<Module, std::string> map
        = {{Module::core, "core"}, {Module::http, "http"}};
      assert(map.contains(module));
      auto logger = std::make_shared<spdlog::logger>(
        map[module], sinks.begin(), sinks.end());
      logger->set_level(spdlog::level::info); // default level
      logger->flush_on(spdlog::level::warn);
      spdlog::register_logger(logger);
      return logger;
    }
  } // namespace

  namespace detail
  {
    std::shared_ptr<spdlog::logger> core()
    {
      static auto logger = MakeLogger(Module::core);
      return logger;
    }

    std::shared_ptr<spdlog::logger> http()
    {
      static auto logger = MakeLogger(Module::http);
      return logger;
    }

  } // namespace detail

  void Init()
  {
    detail::core();
    detail::http();
  }

  void SetLevel(Module module, spdlog::level::level_enum level)
  {
    switch(module)
      {
      case Module::core: detail::core()->set_level(level); return;
      case Module::http: detail::http()->set_level(level); return;
      }
  }

  void SetAllLevels(spdlog::level::level_enum level)
  {
    for(auto module : magic_enum::enum_values<Module>())
      {
        SetLevel(module, level);
      }
  }

} // namespace proxypp::log
