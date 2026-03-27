#pragma once

#include "common/error_code.hpp"

#include <boost/json.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/string_body.hpp>

#include <optional>
#include <string>
#include <string_view>

namespace http_response_util{
    using request_type = boost::beast::http::request<boost::beast::http::string_body>;
    using response_type = boost::beast::http::response<boost::beast::http::string_body>;

    response_type create_text(
        const request_type& request,
        boost::beast::http::status status,
        std::string body
    );
    response_type create_json(
        const request_type& request,
        boost::beast::http::status status,
        const boost::json::value& response_value
    );
    response_type create_error(
        const request_type& request,
        boost::beast::http::status status,
        std::string_view code,
        std::string_view message,
        std::optional<std::string> field_opt = std::nullopt
    );
    response_type create_bearer_error(
        const request_type& request,
        std::string_view code,
        std::string_view message
    );
    response_type create_4xx_or_500(
        const request_type& request,
        std::string_view action,
        const error_code& code
    );
    response_type create_404_or_500(
        const request_type& request,
        std::string_view action,
        const error_code& code
    );
    response_type create_bearer_unauthorized(
        const request_type& request,
        std::string body
    );
    response_type create_method_not_allowed(
        const request_type& request
    );
    response_type create_not_found(
        const request_type& request
    );
}
