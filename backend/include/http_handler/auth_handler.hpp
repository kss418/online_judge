#pragma once

#include "common/db_connection.hpp"

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/string_body.hpp>

namespace auth_handler{
    using request_type = boost::beast::http::request<boost::beast::http::string_body>;
    using response_type = boost::beast::http::response<boost::beast::http::string_body>;

    response_type post_sign_up(
        const request_type& request,
        db_connection& db_connection_value
    );
    response_type post_login(
        const request_type& request,
        db_connection& db_connection_value
    );
    response_type post_token_renew(
        const request_type& request,
        db_connection& db_connection_value
    );
    response_type post_logout(
        const request_type& request,
        db_connection& db_connection_value
    );
}
