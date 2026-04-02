#include "error/http_error.hpp"

#include <array>
#include <cstddef>
#include <utility>

namespace{
    struct http_error_spec{
        std::string_view code_string;
        std::string_view default_message;
        boost::beast::http::status status;
        bool requires_bearer_auth = false;
    };

    using status = boost::beast::http::status;

    constexpr std::array http_error_specs{
        http_error_spec{
            "bad_request",
            "bad request",
            status::bad_request
        },
        http_error_spec{
            "payload_too_large",
            "request body too large",
            status::payload_too_large
        },
        http_error_spec{
            "method_not_allowed",
            "method not allowed",
            status::method_not_allowed
        },
        http_error_spec{
            "validation_error",
            "validation error",
            status::bad_request
        },
        http_error_spec{
            "unauthorized",
            "unauthorized",
            status::unauthorized
        },
        http_error_spec{
            "forbidden",
            "forbidden",
            status::forbidden
        },
        http_error_spec{
            "not_found",
            "not found",
            status::not_found
        },
        http_error_spec{
            "conflict",
            "conflict",
            status::conflict
        },
        http_error_spec{
            "service_unavailable",
            "service unavailable",
            status::service_unavailable
        },
        http_error_spec{
            "internal_server_error",
            "internal server error",
            status::internal_server_error
        },
        http_error_spec{
            "invalid_json",
            "invalid json",
            status::bad_request
        },
        http_error_spec{
            "invalid_query_string",
            "invalid query string",
            status::bad_request
        },
        http_error_spec{
            "duplicate_query_parameter",
            "duplicate query parameter",
            status::bad_request
        },
        http_error_spec{
            "invalid_query_parameter",
            "invalid query parameter",
            status::bad_request
        },
        http_error_spec{
            "unsupported_query_parameter",
            "unsupported query parameter",
            status::bad_request
        },
        http_error_spec{
            "missing_field",
            "missing field",
            status::bad_request
        },
        http_error_spec{
            "invalid_field",
            "invalid field",
            status::bad_request
        },
        http_error_spec{
            "invalid_length",
            "invalid length",
            status::bad_request
        },
        http_error_spec{
            "invalid_argument",
            "invalid argument",
            status::bad_request
        },
        http_error_spec{
            "missing_or_invalid_bearer_token",
            "missing or invalid bearer token",
            status::unauthorized,
            true
        },
        http_error_spec{
            "invalid_or_expired_token",
            "invalid, expired, or revoked token",
            status::unauthorized,
            true
        },
        http_error_spec{
            "admin_bearer_token_required",
            "admin bearer token required",
            status::unauthorized,
            true
        },
        http_error_spec{
            "superadmin_bearer_token_required",
            "superadmin bearer token required",
            status::unauthorized,
            true
        },
        http_error_spec{
            "invalid_credentials",
            "invalid credentials",
            status::unauthorized
        },
        http_error_spec{
            "submission_banned",
            "submission is currently banned",
            status::forbidden
        },
        http_error_spec{
            "invalid_testcase_zip",
            "invalid testcase zip",
            status::bad_request
        },
    };

    const http_error_spec unknown_http_error_spec{
        "unknown_http_error",
        "unknown http error",
        status::internal_server_error
    };

    const http_error_spec& get_http_error_spec(http_error_code ec){
        const auto index = static_cast<std::size_t>(ec);
        if(index >= http_error_specs.size()){
            return unknown_http_error_spec;
        }

        return http_error_specs[index];
    }

    http_error_code map_service_error_code(const service_error& ec){
        switch(ec.code){
            case service_error_code::validation_error:
                return http_error_code::validation_error;
            case service_error_code::unauthorized:
                return http_error_code::unauthorized;
            case service_error_code::forbidden:
                return http_error_code::forbidden;
            case service_error_code::not_found:
                return http_error_code::not_found;
            case service_error_code::conflict:
                return http_error_code::conflict;
            case service_error_code::unavailable:
            case service_error_code::internal:
                return http_error_code::internal_server_error;
        }

        return http_error_code::internal_server_error;
    }
}

http_error::http_error(
    http_error_code code_value,
    std::string message_value,
    std::optional<std::string> field_opt_value
):
    code(code_value),
    message(
        message_value.empty()
            ? std::string{get_http_error_spec(code_value).default_message}
            : std::move(message_value)
    ),
    field_opt(std::move(field_opt_value)){}

http_error::http_error(const service_error& error)
:
    http_error(from_service_error(error)){}

boost::beast::http::status http_error::status() const{
    return get_http_error_spec(code).status;
}

std::string_view http_error::code_string() const{
    return get_http_error_spec(code).code_string;
}

bool http_error::requires_bearer_auth() const{
    return get_http_error_spec(code).requires_bearer_auth;
}

http_error http_error::from_service_error(const service_error& error){
    return http_error{map_service_error_code(error)};
}

std::string_view to_code_string(http_error_code ec){
    return get_http_error_spec(ec).code_string;
}
