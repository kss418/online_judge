#pragma once

#include "http_core/request_context.hpp"

#include <boost/beast/http/message.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/string_body.hpp>

class system_handler{
public:
    using context_type = request_context;
    using request_type = context_type::request_type;
    using response_type = context_type::response_type;

    response_type handle_health_get(context_type& context);
    response_type handle_supported_languages_get(context_type& context);
};
