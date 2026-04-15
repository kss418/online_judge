#pragma once

#include "http_core/request_context.hpp"
#include "http_guard/guard_runner.hpp"

#include <cstdint>
#include <expected>

namespace user_guard{
    using context_type = request_context;
    using response_type = context_type::response_type;

    std::expected<void, response_type> require_exists(
        context_type& context,
        std::int64_t user_id
    );

    inline auto make_exists_guard(std::int64_t user_id){
        return [user_id](const http_guard::guard_context& context){
            return require_exists(
                context.request_context_ref(),
                user_id
            );
        };
    }
}
