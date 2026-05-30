#pragma once

#include <memory>
#include <spdlog/spdlog.h>

namespace proxypp::log {
    void Init();

    // add more logger for other modules, such as http, socks..
    namespace detail {
        std::shared_ptr<spdlog::logger> core();
    }

} // namespace proxypp::log

#define LOG_CORE_INFO(...) SPDLOG_LOGGER_INFO(::proxypp::log::core(), __VA_ARGS__)
