#pragma once

#include "http_handler/problem_handler.hpp"

#include <cstdint>
#include <string_view>

class problem_router{
public:
    using context_type = problem_handler::context_type;
    using request_type = problem_handler::request_type;
    using response_type = problem_handler::response_type;

    response_type route(context_type& context, std::string_view path);
};
