cmake_minimum_required(VERSION 3.20)

project(online_judge LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

add_library(code_runner STATIC
    src/judge/code_runner.cpp
)

target_include_directories(code_runner
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(runner_main
    src/main/runner_main.cpp
)

target_link_libraries(runner_main
    PRIVATE
        code_runner
)
