#pragma once

#include "common/db_connection.hpp"
#include "http_handler/user_handler.hpp"

#include <cstdint>
#include <string>
#include <string_view>

class user_router{
public:
    using request_type = user_handler::request_type;
    using response_type = user_handler::response_type;

    user_router(const user_router&) = delete;
    user_router& operator=(const user_router&) = delete;
    user_router(user_router&&) noexcept = delete;
    user_router& operator=(user_router&&) noexcept = delete;

    explicit user_router(db_connection& db_connection);
    response_type route(const request_type& request, std::string_view path);

private:
    response_type handle_public_user_list(const request_type& request);
    response_type handle_user_list(const request_type& request);

    response_type handle_user_me(const request_type& request);
    response_type handle_user_me_statistics(const request_type& request);
    response_type handle_user_me_submission_ban(const request_type& request);
    response_type handle_user_me_solved_problems(const request_type& request);
    response_type handle_user_me_wrong_problems(const request_type& request);
    response_type handle_user_summary_by_login_id(
        const request_type& request,
        const std::string& user_login_id
    );
    response_type handle_user_summary(
        const request_type& request,
        std::int64_t user_id
    );
    response_type handle_user_statistics(
        const request_type& request,
        std::int64_t user_id
    );
    response_type handle_user_solved_problems(
        const request_type& request,
        std::int64_t user_id
    );
    response_type handle_user_wrong_problems(
        const request_type& request,
        std::int64_t user_id
    );

    response_type handle_user_admin(
        const request_type& request,
        std::int64_t user_id
    );

    response_type handle_user_regular(
        const request_type& request,
        std::int64_t user_id
    );
    response_type handle_user_submission_ban(
        const request_type& request,
        std::int64_t user_id
    );

    db_connection& db_connection_;
};
