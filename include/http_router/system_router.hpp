#pragma once

#include "http_handler/system_handler.hpp"

#include <string_view>

class system_router{
public:
    using request_type = system_handler::request_type;
    using response_type = system_handler::response_type;

    system_router() = default;
    system_router(const system_router&) = delete;
    system_router& operator=(const system_router&) = delete;
    system_router(system_router&&) noexcept = delete;
    system_router& operator=(system_router&&) noexcept = delete;

    response_type handle(const request_type& request, std::string_view path);

private:
    response_type handle_health(const request_type& request);

    system_handler system_handler_;
};
