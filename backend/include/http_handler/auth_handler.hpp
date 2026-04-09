#pragma once

#include "http_core/request_context.hpp"

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/string_body.hpp>

namespace auth_handler{
    using context_type = request_context;
    using request_type = context_type::request_type;
    using response_type = context_type::response_type;

    response_type post_sign_up(context_type& context);
    response_type post_login(context_type& context);
    response_type post_token_renew(context_type& context);
    response_type post_logout(context_type& context);
}
