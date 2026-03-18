#pragma once

#include "db/auth_service.hpp"
#include "db/db_connection.hpp"

#include <boost/json.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/string_body.hpp>

#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace http_util{
    using request_type = boost::beast::http::request<boost::beast::http::string_body>;
    using response_type = boost::beast::http::response<boost::beast::http::string_body>;

    response_type create_text_response(
        const request_type& request,
        boost::beast::http::status status,
        std::string body
    );
    response_type create_bearer_unauthorized_response(
        const request_type& request,
        std::string body
    );
    response_type method_not_allowed_response(
        const request_type& request
    );
    response_type not_found_response(
        const request_type& request
    );
    std::optional<boost::json::object> parse_json_object(
        const request_type& request
    );
    std::optional<std::string_view> get_string_field(
        const boost::json::object& object,
        std::string_view key
    );
    std::optional<std::string_view> get_non_empty_string_field(
        const boost::json::object& object,
        std::string_view key
    );
    std::optional<std::int64_t> get_positive_int64_field(
        const boost::json::object& object,
        std::string_view key
    );
    std::optional<std::int32_t> get_positive_int32_field(
        const boost::json::object& object,
        std::string_view key
    );
    std::optional<std::vector<std::string_view>> parse_path(
        std::string_view prefix,
        std::string_view path
    );
    std::expected<auth_service::auth_identity, response_type> try_auth_bearer(
        const request_type& request,
        db_connection& db_connection
    );
    std::expected<auth_service::auth_identity, response_type> try_admin_auth_bearer(
        const request_type& request,
        db_connection& db_connection
    );
    std::optional<std::string_view> get_bearer_token(
        const request_type& request
    );
}
