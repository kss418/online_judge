cmake_minimum_required(VERSION 3.20)

project(online_judge LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

find_package(boost_beast CONFIG REQUIRED)

add_library(lib STATIC
    src/http_server/http_handler.cpp
    src/http_server/acceptor.cpp
    src/http_server/http_session.cpp
    src/judge_server/code_runner.cpp
    src/judge_server/compile_runner.cpp
    src/judge_server/judge_utility.cpp
    src/judge_server/judge.cpp
    src/judge_server/checker.cpp
    src/common/error_code.cpp
    src/common/unique_fd.cpp
    src/common/temp_file.cpp
    src/common/blocking_io.cpp
)

target_include_directories(lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(lib
    PUBLIC
        Boost::beast
)

add_executable(http_server
    main/http_server.cpp
)

target_link_libraries(http_server
    PRIVATE
        lib
)
