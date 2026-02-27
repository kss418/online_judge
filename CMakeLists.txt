cmake_minimum_required(VERSION 3.20)

project(online_judge LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

find_package(boost_beast CONFIG REQUIRED)

add_library(lib STATIC
    src/http_server/net/acceptor.cpp
    src/http_server/net/http_session.cpp
    src/judge_server/judge/code_runner.cpp
    src/judge_server/judge/compile_runner.cpp
    src/judge_server/judge/judge_utility.cpp
    src/judge_server/judge/judge.cpp
    src/judge_server/judge/checker.cpp
    src/judge_server/core/error_code.cpp
    src/judge_server/core/unique_fd.cpp
    src/judge_server/core/temp_file.cpp
    src/judge_server/net/blocking_io.cpp
)

target_include_directories(lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(lib
    PUBLIC
        Boost::beast
)

add_executable(runner_main
    src/main/runner_main.cpp
)

target_link_libraries(runner_main
    PRIVATE
        lib
)
