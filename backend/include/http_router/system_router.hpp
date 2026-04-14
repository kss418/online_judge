#pragma once

#include "http_core/request_context.hpp"

#include <string_view>

class system_router{
public:
    using context_type = request_context;
    using request_type = context_type::request_type;
    using response_type = context_type::response_type;

    system_router() = default;
    system_router(const system_router&) = delete;
    system_router& operator=(const system_router&) = delete;
    system_router(system_router&&) noexcept = delete;
    system_router& operator=(system_router&&) noexcept = delete;

    response_type route_public(context_type& context, std::string_view path);
    response_type route(context_type& context, std::string_view path);
};
