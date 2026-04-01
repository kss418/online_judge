add_library(lib STATIC)

add_subdirectory(common)
add_subdirectory(dto)
add_subdirectory(serializer)
add_subdirectory(db_event)
add_subdirectory(db_service)
add_subdirectory(db_repository)
add_subdirectory(query_builder)
add_subdirectory(http_handler)
add_subdirectory(http_router)
add_subdirectory(http_core)
add_subdirectory(http_guard)
add_subdirectory(judge_core)
add_subdirectory(pl_runner)

target_include_directories(lib
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(lib
    PUBLIC
        Boost::beast
        Boost::json
        libpqxx::pqxx
        OpenSSL::Crypto
)
