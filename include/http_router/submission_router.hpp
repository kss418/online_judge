#pragma once

#include "common/db_connection.hpp"
#include "http_handler/submission_handler.hpp"

#include <cstdint>
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
    response_type handle_submissions(const request_type& request);
    response_type handle_submission(
        const request_type& request,
        std::int64_t resource_id
    );
    response_type handle_submission_source(
        const request_type& request,
        std::int64_t resource_id
    );

    db_connection& db_connection_;
};
