#pragma once

#include "http_core/request_context.hpp"

#include <cstdint>
#include <string_view>

class problem_router{
public:
    using context_type = request_context;
    using request_type = context_type::request_type;
    using response_type = context_type::response_type;

    response_type route(context_type& context, std::string_view path);
};
