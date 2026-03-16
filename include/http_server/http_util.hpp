#pragma once

#include <boost/json.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/string_body.hpp>

#include <optional>
#include <string>
#include <string_view>

namespace http_util{
    using request_type = boost::beast::http::request<boost::beast::http::string_body>;
    using response_type = boost::beast::http::response<boost::beast::http::string_body>;

    response_type create_text_response(
        const request_type& request,
        boost::beast::http::status status,
        std::string body
    );
    std::optional<boost::json::object> parse_json_object(
        const request_type& request
    );
    std::optional<std::string_view> get_non_empty_string_field(
        const boost::json::object& object,
        std::string_view key
    );
}
