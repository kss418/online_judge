cmake_minimum_required(VERSION 3.20)

project(online_judge LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

add_library(lib STATIC
    src/judge/code_runner.cpp
    src/judge/compile_runner.cpp
    src/judge/judge_utility.cpp
    src/judge/judge.cpp
    src/judge/checker.cpp
    src/core/error_code.cpp
    src/core/unique_fd.cpp
    src/core/temp_file.cpp
    src/net/blocking_io.cpp
)

target_include_directories(lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(runner_main
    src/main/runner_main.cpp
)

target_link_libraries(runner_main
    PRIVATE
        lib
)
