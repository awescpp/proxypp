cmake_minimum_required(VERSION 3.27)
project(proxy++ LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Boost CONFIG REQUIRED COMPONENTS asio beast date_time url)
find_package(CLI11 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(magic_enum CONFIG REQUIRED)
find_package(qjs CONFIG REQUIRED)

# proxypp::options
add_library(proxypp_options INTERFACE)
add_library(proxypp::options ALIAS proxypp_options)
target_include_directories(proxypp_options INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_precompile_headers(proxypp_options INTERFACE src/proxypp/pch.h)
target_compile_options(proxypp_options INTERFACE $<$<CXX_COMPILER_ID:MSVC>:/utf-8> $<$<CXX_COMPILER_ID:MSVC>:/bigobj>)
target_compile_definitions(proxypp_options INTERFACE
        SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE
        $<$<PLATFORM_ID:Windows>:_WIN32_WINNT=0x0A00>
)
target_link_libraries(proxypp_options INTERFACE Boost::asio Boost::beast Boost::url spdlog::spdlog magic_enum::magic_enum)

# proxypp::log
add_library(proxypp_log STATIC src/proxypp/log/log.cpp)
add_library(proxypp::log ALIAS proxypp_log)
target_link_libraries(proxypp_log PUBLIC proxypp::options)

# proxypp::http
add_library(proxypp_http STATIC
        src/proxypp/http/http_proxy_session.cpp
        src/proxypp/http/http_message_utils.cpp
        src/proxypp/http/body_info.cpp
)
add_library(proxypp::http ALIAS proxypp_http)
target_link_libraries(proxypp_http PUBLIC proxypp::options proxypp::log)

# proxypp::script
add_library(proxypp_script STATIC src/proxypp/script/js_runtime.cpp)
add_library(proxypp::script ALIAS proxypp_script)
target_link_libraries(proxypp_script PUBLIC proxypp::options proxypp::log qjs)

# proxypp::core
add_library(proxypp_core STATIC src/proxypp/core/tcp_server.cpp)
add_library(proxypp::core ALIAS proxypp_core)
target_link_libraries(proxypp_core PUBLIC proxypp::options proxypp::http proxypp::script proxypp::log)

# proxy++ executable
add_executable(proxy++ app/main.cpp)
target_link_libraries(proxy++ PRIVATE CLI11::CLI11 proxypp::core)

include(CTest)
if (BUILD_TESTING)
    add_subdirectory(tests)
endif ()