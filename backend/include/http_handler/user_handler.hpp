#pragma once

#include "http_core/request_context.hpp"

#include <cstdint>
#include <string_view>

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/string_body.hpp>

namespace user_handler{
    using context_type = request_context;
    using request_type = context_type::request_type;
    using response_type = context_type::response_type;

    response_type get_me(context_type& context);
    response_type get_me_submission_statistics(context_type& context);
    response_type get_me_submission_ban(context_type& context);
    response_type get_me_solved_problems(context_type& context);
    response_type get_me_wrong_problems(context_type& context);
    response_type get_public_user_list(context_type& context);
    response_type get_user_summary(
        context_type& context,
        std::int64_t user_id
    );
    response_type get_user_summary_by_login_id(
        context_type& context,
        std::string_view user_login_id
    );
    response_type get_user_submission_statistics(
        context_type& context,
        std::int64_t user_id
    );
    response_type get_user_solved_problems(
        context_type& context,
        std::int64_t user_id
    );
    response_type get_user_wrong_problems(
        context_type& context,
        std::int64_t user_id
    );

    response_type get_user_list(context_type& context);

    response_type put_user_admin(
        context_type& context,
        std::int64_t user_id
    );
    response_type put_user_regular(
        context_type& context,
        std::int64_t user_id
    );
    response_type get_user_submission_ban(
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
