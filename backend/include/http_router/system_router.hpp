#pragma once

#include "http_handler/system_query_handler.hpp"

#include <string_view>

class system_router{
public:
    using context_type = system_query_handler::context_type;
    using request_type = system_query_handler::request_type;
    using response_type = system_query_handler::response_type;

    system_router() = default;
    system_router(const system_router&) = delete;
    system_router& operator=(const system_router&) = delete;
    system_router(system_router&&) noexcept = delete;
    system_router& operator=(system_router&&) noexcept = delete;

    response_type route_public(context_type& context, std::string_view path);
    response_type route(context_type& context, std::string_view path);
};
