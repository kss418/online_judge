#pragma once

#include "http_handler/user_handler.hpp"

#include <cstdint>
#include <string>
#include <string_view>

class user_router{
public:
    using context_type = user_handler::context_type;
    using request_type = user_handler::request_type;
    using response_type = user_handler::response_type;

    user_router(const user_router&) = delete;
    user_router& operator=(const user_router&) = delete;
    user_router(user_router&&) noexcept = delete;
    user_router& operator=(user_router&&) noexcept = delete;

    user_router() = default;
    response_type route(context_type& context, std::string_view path);
};
