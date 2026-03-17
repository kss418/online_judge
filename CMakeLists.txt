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

add_library(lib STATIC
    src/db/db_connection.cpp
    src/db/auth_util.cpp
    src/db/auth_service.cpp
    src/db/login_service.cpp
    src/db/login_util.cpp
    src/db/submission_util.cpp
    src/db/submission_core_service.cpp
    src/db/problem_core_service.cpp
    src/db/problem_content_service.cpp
    src/db/problem_statistics_service.cpp
    src/db/problem_service_util.cpp
    src/db/testcase_service.cpp
    src/db/testcase_service_util.cpp
    src/db/submission_service.cpp
    src/http_handler/auth_handler.cpp
    src/http_handler/problem_handler.cpp
    src/http_handler/submission_handler.cpp
    src/http_handler/system_handler.cpp
    src/http_router/auth_router.cpp
    src/http_server/http_dispatcher.cpp
    src/http_server/http_util.cpp
    src/http_server/http_server.cpp
    src/http_server/acceptor.cpp
    src/http_server/http_session.cpp
    src/common/file_util.cpp
    src/judge_server/testcase_runner.cpp
    src/pl_runner/cpp_runner.cpp
    src/pl_runner/python_runner.cpp
    src/pl_runner/java_runner.cpp
    src/pl_runner/pl_runner_util.cpp
    src/judge_server/sandbox_runner.cpp
    src/judge_server/judge_worker.cpp
    src/judge_server/judge_util.cpp
    src/judge_server/tc_util.cpp
    src/judge_server/checker.cpp
    src/judge_server/testcase_downloader.cpp
    src/common/env_util.cpp
    src/common/error_code.cpp
    src/common/crypto_util.cpp
    src/common/password_util.cpp
    src/common/string_util.cpp
    src/common/token_util.cpp
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
        Boost::json
        libpqxx::pqxx
        OpenSSL::Crypto
)

add_executable(http_server
    main/http_server.cpp
)

target_link_libraries(http_server
    PRIVATE
        lib
)

add_executable(judge_server
    main/judge_server.cpp
)

target_link_libraries(judge_server
    PRIVATE
        lib
)
