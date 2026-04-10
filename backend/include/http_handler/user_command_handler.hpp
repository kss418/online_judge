#pragma once

#include "http_core/request_context.hpp"

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/string_body.hpp>

#include <cstdint>

namespace user_command_handler{
    using context_type = request_context;
    using request_type = context_type::request_type;
    using response_type = context_type::response_type;

    response_type put_user_admin(
        context_type& context,
        std::int64_t user_id
    );
    response_type put_user_regular(
        context_type& context,
        std::int64_t user_id
    );
    response_type post_user_submission_ban(
        context_type& context,
        std::int64_t user_id
    );
    response_type delete_user_submission_ban(
        context_type& context,
        std::int64_t user_id
    );
}
