#pragma once

#include "error/service_error.hpp"

#include <boost/beast/http/status.hpp>

#include <optional>
#include <ostream>
#include <span>
#include <string>
#include <string_view>

enum class http_error_code{
    bad_request,
    payload_too_large,
    method_not_allowed,
    validation_error,
    unauthorized,
    forbidden,
    not_found,
    conflict,
    service_unavailable,
    internal_server_error,
    invalid_json,
    invalid_query_string,
    duplicate_query_parameter,
    invalid_query_parameter,
    unsupported_query_parameter,
    missing_field,
    invalid_field,
    invalid_length,
    invalid_argument,
    missing_or_invalid_bearer_token,
    invalid_or_expired_token,
    admin_bearer_token_required,
    superadmin_bearer_token_required,
    invalid_credentials,
    submission_banned,
    invalid_testcase_zip,
};

struct http_error_spec{
    http_error_code code;
    std::string_view code_string;
    boost::beast::http::status http_status;
    std::string_view default_public_message_en;
    std::string_view frontend_message_ko;
    std::string_view frontend_field_message_ko;
    bool requires_bearer_auth = false;
    bool field_supported = false;
    std::string_view category;
};

struct http_error{
    http_error_code code;
    std::string message;
    std::optional<std::string> field_opt = std::nullopt;

    http_error(
        http_error_code code_value,
        std::string message_value = {},
        std::optional<std::string> field_opt_value = std::nullopt
    );
    http_error(const service_error& error);

    boost::beast::http::status status() const;
    std::string_view code_string() const;
    bool requires_bearer_auth() const;

    static http_error from_service_error(const service_error& error);
};

std::string_view to_code_string(http_error_code ec);
std::span<const http_error_spec> all_http_error_specs();
const http_error_spec* find_http_error_spec(http_error_code code);
