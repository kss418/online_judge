#pragma once

#include "common/db_connection.hpp"
#include "http_handler/user_handler.hpp"

#include <cstdint>
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
    response_type handle_user_list(const request_type& request);

    response_type handle_user_me(const request_type& request);

    response_type handle_user_admin(
        const request_type& request,
        std::int64_t user_id
    );

    response_type handle_user_permission(
        const request_type& request,
        std::int64_t user_id
    );

    db_connection& db_connection_;
};
