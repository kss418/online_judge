#pragma once

#include "db/db_connection.hpp"
#include "http_handler/auth_handler.hpp"

#include <string_view>

class auth_router{
public:
    using request_type = auth_handler::request_type;
    using response_type = auth_handler::response_type;

    auth_router(const auth_router&) = delete;
    auth_router& operator=(const auth_router&) = delete;
    auth_router(auth_router&&) noexcept = delete;
    auth_router& operator=(auth_router&&) noexcept = delete;

    explicit auth_router(db_connection& db_connection);
    response_type handle(const request_type& request, std::string_view path);

private:
    response_type handle_sign_up(const request_type& request);
    response_type handle_login(const request_type& request);
    response_type handle_token_renew(const request_type& request);
    response_type handle_logout(const request_type& request);

    db_connection& db_connection_;
};
