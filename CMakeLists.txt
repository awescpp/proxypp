cmake_minimum_required(VERSION 3.27)
project(proxypp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(PROXYPP_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(PROXYPP_BUILD_APP "Build proxy++ executable" ON)

find_package(Boost CONFIG REQUIRED COMPONENTS asio beast date_time url json)
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
target_precompile_headers(proxypp_options INTERFACE src/proxypp/pch.h)
target_compile_options(proxypp_options INTERFACE $<$<CXX_COMPILER_ID:MSVC>:/utf-8> $<$<CXX_COMPILER_ID:MSVC>:/bigobj>)
target_compile_definitions(proxypp_options INTERFACE
        SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE
        $<$<PLATFORM_ID:Windows>:_WIN32_WINNT=0x0A00>
)
target_link_libraries(proxypp_options INTERFACE
        Boost::asio
        Boost::beast
        Boost::json
        Boost::url
        spdlog::spdlog
        magic_enum::magic_enum
        tl::expected
)
if (PROXYPP_ENABLE_ASAN)
    if (MSVC)
        target_compile_options(proxypp_options INTERFACE /fsanitize=address)
        target_link_options(proxypp_options INTERFACE /INCREMENTAL:NO)
    else ()
        target_compile_options(proxypp_options INTERFACE -fsanitize=address -fno-omit-frame-pointer)
        target_link_options(proxypp_options INTERFACE -fsanitize=address)
    endif ()
endif ()


#proxypp::common
add_library(proxypp_common STATIC src/proxypp/error.cpp)
add_library(proxypp::common ALIAS proxypp_common)
target_link_libraries(proxypp_common PUBLIC proxypp::options)

# proxypp::log
add_library(proxypp_log STATIC src/proxypp/log/log.cpp)
add_library(proxypp::log ALIAS proxypp_log)
target_link_libraries(proxypp_log PUBLIC proxypp::options proxypp::common)

# proxypp::http
add_library(proxypp_http STATIC
        src/proxypp/http/adapter/beast_request_adapter.cpp
        src/proxypp/http/adapter/beast_response_adapter.cpp
        src/proxypp/http/http_proxy_session.cpp
        src/proxypp/http/http_message_utils.cpp
        src/proxypp/http/body_info.cpp
)
add_library(proxypp::http ALIAS proxypp_http)
target_link_libraries(proxypp_http PUBLIC proxypp::options proxypp::log proxypp::common)

# proxypp::rule
add_library(proxypp_rule STATIC
        src/proxypp/rule/config.cpp
        src/proxypp/rule/error.cpp
        src/proxypp/rule/op.cpp
        src/proxypp/rule/match.cpp
        src/proxypp/rule/schema_validator.cpp
        src/proxypp/rule/rule_engine.cpp
        src/proxypp/rule/match_context.cpp
        src/proxypp/rule/http/model.cpp
        src/proxypp/rule/http/action_executor.cpp
        src/proxypp/rule/http/context_injector.cpp
        src/proxypp/rule/http/apply.cpp
)
add_library(proxypp::rule ALIAS proxypp_rule)
target_link_libraries(proxypp_rule PUBLIC proxypp_options proxypp::log proxypp::common PRIVATE jsoncons::jsoncons)

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
target_link_libraries(proxypp_script PUBLIC proxypp::options proxypp::log proxypp::common qjs)

# proxypp::core
add_library(proxypp_core STATIC src/proxypp/core/tcp_server.cpp)
add_library(proxypp::core ALIAS proxypp_core)
target_link_libraries(proxypp_core PUBLIC
        proxypp::options
        proxypp::common
        proxypp::http
        proxypp::script
        proxypp_rule
        proxypp::log
)

# proxy++ executable
if (PROXYPP_BUILD_APP)
    add_executable(proxy++ app/main.cpp)
    target_link_libraries(proxy++ PRIVATE CLI11::CLI11 proxypp::core)
endif ()

include(CTest)
if (BUILD_TESTING)
    add_subdirectory(tests)
endif ()