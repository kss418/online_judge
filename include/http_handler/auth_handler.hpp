#pragma once

#include "db/db_connection.hpp"

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/string_body.hpp>

namespace auth_handler{
    using request_type = boost::beast::http::request<boost::beast::http::string_body>;
    using response_type = boost::beast::http::response<boost::beast::http::string_body>;

    response_type handle_sign_up_post(
        const request_type& request,
        db_connection& db_connection_value
    );
    response_type handle_login_post(
        const request_type& request,
        db_connection& db_connection_value
    );
    response_type handle_token_renew_post(
        const request_type& request,
        db_connection& db_connection_value
    );
    response_type handle_logout_post(
        const request_type& request,
        db_connection& db_connection_value
    );
}
