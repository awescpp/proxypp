cmake_minimum_required(VERSION 3.27)
project(proxy++ LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Boost CONFIG REQUIRED COMPONENTS asio beast date_time)
find_package(CLI11 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)


add_executable(proxy++ app/main.cpp src/tcp_server.cpp)
target_precompile_headers(proxy++ PRIVATE src/pch.h)
target_compile_options(proxy++ PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
target_compile_definitions(proxy++ PRIVATE _WIN32_WINNT=0x0A00)
target_link_libraries(proxy++ PRIVATE CLI11::CLI11 Boost::asio Boost::beast Boost::date_time spdlog::spdlog)

include(CTest)
if (BUILD_TESTING)
    add_subdirectory(tests)
endif ()