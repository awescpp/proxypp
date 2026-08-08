cmake_minimum_required(VERSION 3.27)
project(proxypp LANGUAGES CXX VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

option(PROXYPP_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(PROXYPP_ENABLE_UBSAN "Enable UndefinedBehaviourSanitizer" OFF)
option(PROXYPP_ENABLE_COVERAGE "Enable GCC gcov code coverage" OFF)
option(PROXYPP_BUILD_APP "Build proxy++ executable" ON)

if (PROXYPP_ENABLE_COVERAGE AND (PROXYPP_ENABLE_ASAN OR PROXYPP_ENABLE_UBSAN))
    message(FATAL_ERROR "Code coverage and sanitizers cannot be enabled together")
endif ()

find_package(Boost CONFIG REQUIRED COMPONENTS asio beast date_time url dll)
find_package(CLI11 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(magic_enum CONFIG REQUIRED)
find_package(qjs CONFIG REQUIRED)
find_package(tl-expected CONFIG REQUIRED)
find_package(jsoncons CONFIG REQUIRED)

# proxypp::options
add_library(proxypp_options INTERFACE)
add_library(proxypp::options ALIAS proxypp_options)
target_include_directories(proxypp_options INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_options(proxypp_options INTERFACE $<$<CXX_COMPILER_ID:MSVC>:/utf-8> $<$<CXX_COMPILER_ID:MSVC>:/bigobj>)
target_compile_definitions(proxypp_options INTERFACE
        SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE
        $<$<PLATFORM_ID:Windows>:_WIN32_WINNT=0x0A00>
)
target_link_libraries(proxypp_options INTERFACE
        Boost::asio
        Boost::beast
        Boost::url
        Boost::dll
        spdlog::spdlog
        magic_enum::magic_enum
        tl::expected
)
if (MSVC)
    if (PROXYPP_ENABLE_UBSAN)
        message(FATAL_ERROR "PROXYPP_ENABLE_UBSAN is not supported by MSVC")
    endif ()
    if (PROXYPP_ENABLE_ASAN)
        target_compile_options(proxypp_options INTERFACE /fsanitize=address /Zi)
        target_link_options(proxypp_options INTERFACE /DEBUG /INCREMENTAL:NO)
    endif ()
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(PROXYPP_ENABLED_SANITIZERS)
    if (PROXYPP_ENABLE_ASAN)
        list(APPEND PROXYPP_ENABLED_SANITIZERS address)
    endif ()
    if (PROXYPP_ENABLE_UBSAN)
        list(APPEND PROXYPP_ENABLED_SANITIZERS undefined)
    endif ()
    if (PROXYPP_ENABLED_SANITIZERS)
        list(JOIN PROXYPP_ENABLED_SANITIZERS "," PROXYPP_SANITIZER_LIST)
        target_compile_options(proxypp_options INTERFACE "-fsanitize=${PROXYPP_SANITIZER_LIST}" -fno-omit-frame-pointer)
        target_link_libraries(proxypp_options INTERFACE "-fsanitize=${PROXYPP_SANITIZER_LIST}")
    endif ()
    if (PROXYPP_ENABLE_UBSAN)
        target_compile_options(proxypp_options INTERFACE -fno-sanitize-recover=undefined)
    endif ()
elseif (PROXYPP_ENABLE_ASAN OR PROXYPP_ENABLE_UBSAN)
    message(FATAL_ERROR "Sanitizers are not supported by the current compiler")
endif ()

if (PROXYPP_ENABLE_COVERAGE)
    if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        message(FATAL_ERROR "PROXYPP_ENABLE_COVERAGE currently requires GCC")
    endif ()
    target_compile_options(proxypp_options INTERFACE --coverage -fprofile-abs-path -O0 -g)
    target_link_options(proxypp_options INTERFACE --coverage)
endif ()

#proxypp::common
add_library(proxypp_common STATIC src/proxypp/error.cpp
        src/proxypp/helper/json_helper.cpp
        src/proxypp/helper/file_helper.cpp
        src/proxypp/helper/json_schema_helper.cpp
        src/proxypp/helper/cli_helper.cpp
)
add_library(proxypp::common ALIAS proxypp_common)
target_link_libraries(proxypp_common PUBLIC proxypp::options jsoncons::jsoncons CLI11::CLI11)

# proxypp::log
add_library(proxypp_log STATIC src/proxypp/log/log.cpp)
add_library(proxypp::log ALIAS proxypp_log)
target_link_libraries(proxypp_log PUBLIC proxypp::options proxypp::common)

# proxypp::script
add_library(proxypp_script STATIC
        src/proxypp/script/qjs/runtime.cpp
        src/proxypp/script/qjs/context.cpp
        src/proxypp/script/qjs/value.cpp
        src/proxypp/script/qjs/evaluator.cpp
        src/proxypp/script/qjs/error.cpp
        src/proxypp/script/qjs/object_builder.cpp
        src/proxypp/script/qjs/detail/exception_message.cpp
)
add_library(proxypp::script ALIAS proxypp_script)
target_link_libraries(proxypp_script PUBLIC proxypp::options proxypp::common proxypp::log qjs)

# proxypp::rule
add_library(proxypp_rule STATIC
        src/proxypp/rule/config.cpp
        src/proxypp/rule/error.cpp
        src/proxypp/rule/op.cpp
        src/proxypp/rule/match.cpp
        src/proxypp/rule/rule_engine.cpp
        src/proxypp/rule/match_context.cpp
        src/proxypp/rule/http/model.cpp
        src/proxypp/rule/http/action_executor.cpp
        src/proxypp/rule/http/context_injector.cpp
        src/proxypp/rule/http/apply.cpp
        src/proxypp/rule/rule_file.cpp
)
add_library(proxypp::rule ALIAS proxypp_rule)
target_link_libraries(proxypp_rule PUBLIC proxypp::options proxypp::common proxypp::log proxypp::script)

# proxypp::http
set(PROXYPP_PCH "${CMAKE_SOURCE_DIR}/src/proxypp/pch.h")
add_library(proxypp_http STATIC
        src/proxypp/http/adapter/beast_request_adapter.cpp
        src/proxypp/http/adapter/beast_response_adapter.cpp
        src/proxypp/http/http_proxy_session.cpp
        src/proxypp/http/http_message_utils.cpp
        src/proxypp/http/body_info.cpp
)
add_library(proxypp::http ALIAS proxypp_http)
target_precompile_headers(proxypp_http PRIVATE "${PROXYPP_PCH}")
target_link_libraries(proxypp_http PUBLIC proxypp::options proxypp::common proxypp::log proxypp::rule)

# proxypp::core
add_library(proxypp_core STATIC src/proxypp/core/tcp_server.cpp)
add_library(proxypp::core ALIAS proxypp_core)
target_link_libraries(proxypp_core PUBLIC
        proxypp::options
        proxypp::common
        proxypp::log
        proxypp::script
        proxypp::rule
        proxypp::http
)

execute_process(COMMAND git rev-parse HEAD OUTPUT_VARIABLE GIT_COMMIT OUTPUT_STRIP_TRAILING_WHITESPACE)

configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.h.in"
        "${CMAKE_CURRENT_BINARY_DIR}/generated/include/proxypp/version.h"
        @ONLY
)

# proxy++ executable
if (PROXYPP_BUILD_APP)
    add_executable(proxy++ app/main.cpp)
    target_include_directories(proxy++ PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated/include")
    target_link_libraries(proxy++ PRIVATE proxypp::core CLI11::CLI11)
endif ()

include(CTest)
if (BUILD_TESTING)
    add_subdirectory(tests)
endif ()

set(CPACK_SOURCE_GENERATOR "ZIP;TGZ")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "proxypp-${PROJECT_VERSION}-src")
set(CPACK_SOURCE_IGNORE_FILES
        "/\\.git/"
        "/\\.github/"
        "/out/"
        "/logs/"
        "/build/"
        "/node_modules/"
        "/\\.idea/"
        "/\\.vscode/"
        "/docs/node_modules/"
        "/docs/.astro/"
        "/docs/.vscode/"
)
include(CPack)
