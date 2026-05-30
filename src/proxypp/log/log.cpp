#include "log.h"
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace proxypp::log {
    // Anonymous namespace: used only inside log.cpp, hidden from external code, and not visible during linking.
    namespace {
        std::vector<spdlog::sink_ptr> MakeSinks() {
            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            console_sink->set_pattern("[%H:%M:%S.%e] [%n] [%^%l%$] [%s:%#] %v");

            // TODO: Temporarily disable file_sink for now.
            auto file_sink =
                    std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logs/proxypp.log", 1024 * 1024 * 1024, 5);
            file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] [%t] [%s:%#] %v");

            return {console_sink};
        }

        std::shared_ptr<spdlog::logger> MakeLogger(const std::string &name) {
            auto sinks = MakeSinks();
            auto logger = std::make_shared<spdlog::logger>(name, sinks.begin(), sinks.end());
            logger->set_level(spdlog::level::info);
            logger->flush_on(spdlog::level::warn);
            spdlog::register_logger(logger);
            return logger;
        }
    } // namespace

    namespace detail {
        std::shared_ptr<spdlog::logger> core() {
            static auto logger = MakeLogger("core");
            return logger;
        }
    } // namespace detail

    void Init() { detail::core(); }

} // namespace proxypp::log
