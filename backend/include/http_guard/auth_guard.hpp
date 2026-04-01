#pragma once

#include "common/db_connection.hpp"
#include "dto/auth_dto.hpp"
#include "http_core/http_response_util.hpp"
#include "http_guard/guard_runner.hpp"

#include <expected>
#include <optional>
#include <string_view>

namespace auth_guard{
    using request_type = http_response_util::request_type;
    using response_type = http_response_util::response_type;

    std::optional<std::string_view> get_bearer_token(
        const request_type& request
    );
    std::expected<auth_dto::token, response_type> parse_bearer_token_or_401(
        const request_type& request
    );
    std::expected<auth_dto::identity, response_type> require_auth(
        const request_type& request,
        db_connection& db_connection
    );
    std::expected<std::optional<auth_dto::identity>, response_type> require_optional_auth(
        const request_type& request,
        db_connection& db_connection
    );
    std::expected<auth_dto::identity, response_type> require_admin(
        const request_type& request,
        db_connection& db_connection
    );
    std::expected<auth_dto::identity, response_type> require_superadmin(
        const request_type& request,
        db_connection& db_connection
    );

    inline auto make_auth_guard(){
        return [](const http_guard::guard_context& context){
            return require_auth(
                context.request,
                context.db_connection_value
            );
        };
    }

    inline auto make_optional_auth_guard(){
        return [](const http_guard::guard_context& context){
            return require_optional_auth(
                context.request,
                context.db_connection_value
            );
        };
    }

    inline auto make_admin_guard(){
        return [](const http_guard::guard_context& context){
            return require_admin(
                context.request,
                context.db_connection_value
            );
        };
    }

    inline auto make_superadmin_guard(){
        return [](const http_guard::guard_context& context){
            return require_superadmin(
                context.request,
                context.db_connection_value
            );
        };
    }
}
