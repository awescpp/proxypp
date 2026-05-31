cmake_minimum_required(VERSION 3.27)
project(proxy++ LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Boost CONFIG REQUIRED COMPONENTS asio beast date_time)
find_package(CLI11 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(magic_enum CONFIG REQUIRED)

add_library(proxypp_log STATIC src/proxypp/log/log.cpp)
add_library(proxypp::log ALIAS proxypp_log)
target_compile_definitions(proxypp_log PUBLIC SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE)
target_link_libraries(proxypp_log PUBLIC spdlog::spdlog magic_enum::magic_enum)

add_executable(proxy++ app/main.cpp src/proxypp/core/tcp_server.cpp src/proxypp/http/http_proxy_session.cpp)
target_include_directories(proxy++ PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_precompile_headers(proxy++ PRIVATE src/proxypp/pch.h)
target_compile_options(proxy++ PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
target_compile_definitions(proxy++ PRIVATE _WIN32_WINNT=0x0A00)
target_link_libraries(proxy++ PRIVATE CLI11::CLI11 proxypp::log Boost::asio Boost::beast Boost::date_time)

include(CTest)
if (BUILD_TESTING)
    add_subdirectory(tests)
endif ()