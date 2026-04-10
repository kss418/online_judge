#pragma once

#include "http_core/request_context.hpp"

#include <cstdint>
#include <string>
#include <string_view>

class user_router{
public:
    using context_type = request_context;
    using request_type = context_type::request_type;
    using response_type = context_type::response_type;

    user_router(const user_router&) = delete;
    user_router& operator=(const user_router&) = delete;
    user_router(user_router&&) noexcept = delete;
    user_router& operator=(user_router&&) noexcept = delete;

    user_router() = default;
    response_type route(context_type& context, std::string_view path);
};
