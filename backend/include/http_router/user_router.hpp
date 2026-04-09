#pragma once

#include "http_handler/user_handler.hpp"

#include <cstdint>
#include <string>
#include <string_view>

class user_router{
public:
    using context_type = user_handler::context_type;
    using request_type = user_handler::request_type;
    using response_type = user_handler::response_type;

    user_router(const user_router&) = delete;
    user_router& operator=(const user_router&) = delete;
    user_router(user_router&&) noexcept = delete;
    user_router& operator=(user_router&&) noexcept = delete;

    user_router() = default;
    response_type route(context_type& context, std::string_view path);

private:
    response_type handle_public_user_list(context_type& context);
    response_type handle_user_list(context_type& context);

    response_type handle_user_me(context_type& context);
    response_type handle_user_me_statistics(context_type& context);
    response_type handle_user_me_submission_ban(context_type& context);
    response_type handle_user_me_solved_problems(context_type& context);
    response_type handle_user_me_wrong_problems(context_type& context);
    response_type handle_user_summary_by_login_id(
        context_type& context,
        const std::string& user_login_id
    );
    response_type handle_user_summary(
        context_type& context,
        std::int64_t user_id
    );
    response_type handle_user_statistics(
        context_type& context,
        std::int64_t user_id
    );
    response_type handle_user_solved_problems(
        context_type& context,
        std::int64_t user_id
    );
    response_type handle_user_wrong_problems(
        context_type& context,
        std::int64_t user_id
    );

    response_type handle_user_admin(
        context_type& context,
        std::int64_t user_id
    );

    response_type handle_user_regular(
        context_type& context,
        std::int64_t user_id
    );
    response_type handle_user_submission_ban(
        context_type& context,
        std::int64_t user_id
    );
};
