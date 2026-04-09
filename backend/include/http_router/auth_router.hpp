#pragma once

#include "http_handler/auth_handler.hpp"

#include <string_view>

class auth_router{
public:
    using context_type = auth_handler::context_type;
    using request_type = auth_handler::request_type;
    using response_type = auth_handler::response_type;

    response_type route(context_type& context, std::string_view path);

private:
    response_type handle_sign_up(context_type& context);
    response_type handle_login(context_type& context);
    response_type handle_token_renew(context_type& context);
    response_type handle_logout(context_type& context);
};
