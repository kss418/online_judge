#pragma once

#include "dto/user_dto.hpp"
#include "http_core/request_context.hpp"
#include "http_guard/guard_runner.hpp"

#include <cstdint>
#include <expected>
#include <string_view>

namespace user_guard{
    using context_type = request_context;
    using request_type = context_type::request_type;
    using response_type = context_type::response_type;

    std::expected<user_dto::summary, response_type> require_summary(
        context_type& context,
        std::int64_t user_id
    );
    std::expected<user_dto::summary, response_type> require_summary_by_login_id(
        context_type& context,
        std::string_view user_login_id
    );

    inline auto make_summary_guard(std::int64_t user_id){
        return [user_id](const http_guard::guard_context& context){
            return require_summary(
                context.request_context_value,
                user_id
            );
        };
    }

    inline auto make_summary_by_login_id_guard(std::string_view user_login_id){
        return [user_login_id](const http_guard::guard_context& context){
            return require_summary_by_login_id(
                context.request_context_value,
                user_login_id
            );
        };
    }
}
