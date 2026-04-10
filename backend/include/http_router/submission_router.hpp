#pragma once

#include "http_handler/submission_handler.hpp"

#include <cstdint>
#include <string_view>

class submission_router{
public:
    using context_type = submission_handler::context_type;
    using request_type = submission_handler::request_type;
    using response_type = submission_handler::response_type;

    response_type route(context_type& context, std::string_view path);
};
