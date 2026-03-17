#pragma once

#include "db/db_connection.hpp"
#include "http_handler/submission_handler.hpp"

#include <string_view>

class submission_router{
public:
    using request_type = submission_handler::request_type;
    using response_type = submission_handler::response_type;

    submission_router(const submission_router&) = delete;
    submission_router& operator=(const submission_router&) = delete;
    submission_router(submission_router&&) noexcept = delete;
    submission_router& operator=(submission_router&&) noexcept = delete;

    explicit submission_router(db_connection& db_connection);
    response_type route(const request_type& request, std::string_view path);

private:
    response_type handle_create_submission(const request_type& request);

    db_connection& db_connection_;
};
