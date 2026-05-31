#pragma once

#include <memory>
#include <spdlog/spdlog.h>

namespace proxypp::log {
    enum class Module { core, http };

    void Init();

    void SetLevel(Module module, spdlog::level::level_enum);

    void SetAllLevels(spdlog::level::level_enum);

    // add more logger for other modules, such as http, socks..
    namespace detail {
        std::shared_ptr<spdlog::logger> core();
        std::shared_ptr<spdlog::logger> http();
    } // namespace detail

} // namespace proxypp::log

// region Core Module Loggers

#define LOG_CORE_TRACE(...) SPDLOG_LOGGER_TRACE(::proxypp::log::detail::core(), __VA_ARGS__)

#define LOG_CORE_DEBUG(...) SPDLOG_LOGGER_DEBUG(::proxypp::log::detail::core(), __VA_ARGS__)

#define LOG_CORE_INFO(...) SPDLOG_LOGGER_INFO(::proxypp::log::detail::core(), __VA_ARGS__)

#define LOG_CORE_ERROR(...) SPDLOG_LOGGER_ERROR(::proxypp::log::detail::core(), __VA_ARGS__)

// endregion

// region Http Module Loggers

#define LOG_HTTP_TRACE(...) SPDLOG_LOGGER_TRACE(::proxypp::log::detail::http(), __VA_ARGS__)

#define LOG_HTTP_DEBUG(...) SPDLOG_LOGGER_DEBUG(::proxypp::log::detail::http(), __VA_ARGS__)

#define LOG_HTTP_INFO(...) SPDLOG_LOGGER_INFO(::proxypp::log::detail::http(), __VA_ARGS__)

#define LOG_HTTP_ERROR(...) SPDLOG_LOGGER_ERROR(::proxypp::log::detail::http(), __VA_ARGS__)

// endregion
