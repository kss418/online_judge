cmake_minimum_required(VERSION 3.20)

project(online_judge LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

find_package(boost_beast CONFIG REQUIRED)
find_package(boost_json CONFIG REQUIRED)
find_package(libpqxx CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)

add_subdirectory(src)
add_subdirectory(main)
