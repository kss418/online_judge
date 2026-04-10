#pragma once

#include "error/http_error.hpp"
#include "error/service_error.hpp"

#include <boost/json.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/string_body.hpp>

#include <string_view>

namespace http_response_util{
    using request_type = boost::beast::http::request<boost::beast::http::string_body>;
    using response_type = boost::beast::http::response<boost::beast::http::string_body>;

    response_type create_json(
        const request_type& request,
        boost::beast::http::status status,
        const boost::json::value& response_value
    );
    response_type create_message(
        const request_type& request,
        boost::beast::http::status status,
        std::string_view message
    );
    response_type create_error(
        const request_type& request,
        const http_error& error
    );
    response_type create_4xx_or_500(
        const request_type& request,
        const service_error& code
    );
    response_type create_internal_server_error(
        const request_type& request,
        std::string_view context,
        std::string_view detail = {}
    );
    response_type create_service_unavailable(
        const request_type& request,
        std::string_view message = {}
    );
    response_type create_method_not_allowed(
        const request_type& request
    );
    response_type create_not_found(
        const request_type& request
    );
}
