#pragma once

#include "common/db_connection.hpp"

#include <string_view>

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/string_body.hpp>

namespace user_handler{
    using request_type = boost::beast::http::request<boost::beast::http::string_body>;
    using response_type = boost::beast::http::response<boost::beast::http::string_body>;

    response_type get_me(
        const request_type& request,
        db_connection& db_connection_value
    );
    response_type get_me_submission_statistics(
        const request_type& request,
        db_connection& db_connection_value
    );
    response_type get_me_solved_problems(
        const request_type& request,
        db_connection& db_connection_value
    );
    response_type get_me_wrong_problems(
        const request_type& request,
        db_connection& db_connection_value
    );
    response_type get_user_summary(
        const request_type& request,
        db_connection& db_connection_value,
        std::int64_t user_id
    );
    response_type get_user_summary_by_login_id(
        const request_type& request,
        db_connection& db_connection_value,
        std::string_view user_login_id
    );
    response_type get_user_submission_statistics(
        const request_type& request,
        db_connection& db_connection_value,
        std::int64_t user_id
    );
    response_type get_user_solved_problems(
        const request_type& request,
        db_connection& db_connection_value,
        std::int64_t user_id
    );
    response_type get_user_wrong_problems(
        const request_type& request,
        db_connection& db_connection_value,
        std::int64_t user_id
    );

    response_type get_user_list(
        const request_type& request,
        db_connection& db_connection_value
    );

    response_type put_user_admin(
        const request_type& request,
        db_connection& db_connection_value,
        std::int64_t user_id
    );
    response_type put_user_regular(
        const request_type& request,
        db_connection& db_connection_value,
        std::int64_t user_id
    );
}
