#pragma once

#include "error/service_error.hpp"

#include <boost/beast/http/status.hpp>

#include <optional>
#include <ostream>
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
    invalid_sample_delete_request,
    problem_not_found,
    user_not_found,
    testcase_not_found
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

    bool operator==(const http_error& other) const;
    boost::beast::http::status status() const;
    std::string_view code_string() const;
    bool requires_bearer_auth() const;

    static http_error from_service_error(const service_error& error);
};

boost::beast::http::status to_status(http_error_code ec);
std::string_view to_code_string(http_error_code ec);
bool requires_bearer_auth(http_error_code ec);
std::string to_string(http_error_code ec);
std::string to_string(const http_error& ec);
