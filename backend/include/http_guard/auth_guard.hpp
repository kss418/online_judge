#pragma once

#include "dto/auth_dto.hpp"
#include "http_core/request_context.hpp"
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
    std::expected<auth_dto::token, response_type> parse_bearer_token_or_401(
        const request_context& context
    );
    std::expected<auth_dto::identity, response_type> require_auth(
        request_context& context
    );
    std::expected<std::optional<auth_dto::identity>, response_type> require_optional_auth(
        request_context& context
    );
    std::expected<auth_dto::identity, response_type> require_admin(
        request_context& context
    );
    std::expected<auth_dto::identity, response_type> require_superadmin(
        request_context& context
    );

    inline std::optional<std::int64_t> get_viewer_user_id(
        const std::optional<auth_dto::identity>& auth_identity_opt
    ){
        if(!auth_identity_opt){
            return std::nullopt;
        }

        return auth_identity_opt->user_id;
    }

    inline auto make_auth_guard(){
        return [](const http_guard::guard_context& context){
            return require_auth(context.request_context_ref());
        };
    }

    inline auto make_bearer_token_guard(){
        return [](const http_guard::guard_context& context){
            return parse_bearer_token_or_401(context.request_context_ref());
        };
    }

    inline auto make_optional_auth_guard(){
        return [](const http_guard::guard_context& context){
            return require_optional_auth(context.request_context_ref());
        };
    }

    inline auto make_admin_guard(){
        return [](const http_guard::guard_context& context){
            return require_admin(context.request_context_ref());
        };
    }

    inline auto make_superadmin_guard(){
        return [](const http_guard::guard_context& context){
            return require_superadmin(context.request_context_ref());
        };
    }
}
