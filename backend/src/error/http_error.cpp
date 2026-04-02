#include "error/http_error.hpp"

#include <utility>

namespace{
    boost::beast::http::status to_status_value(http_error_code ec);
    bool requires_bearer_auth_value(http_error_code ec);

    std::string default_message(http_error_code ec){
        switch(ec){
            case http_error_code::bad_request:
                return "bad request";
            case http_error_code::payload_too_large:
                return "request body too large";
            case http_error_code::method_not_allowed:
                return "method not allowed";
            case http_error_code::validation_error:
                return "validation error";
            case http_error_code::unauthorized:
                return "unauthorized";
            case http_error_code::forbidden:
                return "forbidden";
            case http_error_code::not_found:
                return "not found";
            case http_error_code::conflict:
                return "conflict";
            case http_error_code::service_unavailable:
                return "service unavailable";
            case http_error_code::internal_server_error:
                return "internal server error";
            case http_error_code::invalid_json:
                return "invalid json";
            case http_error_code::invalid_query_string:
                return "invalid query string";
            case http_error_code::duplicate_query_parameter:
                return "duplicate query parameter";
            case http_error_code::invalid_query_parameter:
                return "invalid query parameter";
            case http_error_code::unsupported_query_parameter:
                return "unsupported query parameter";
            case http_error_code::missing_field:
                return "missing field";
            case http_error_code::invalid_field:
                return "invalid field";
            case http_error_code::invalid_length:
                return "invalid length";
            case http_error_code::invalid_argument:
                return "invalid argument";
            case http_error_code::missing_or_invalid_bearer_token:
                return "missing or invalid bearer token";
            case http_error_code::invalid_or_expired_token:
                return "invalid, expired, or revoked token";
            case http_error_code::admin_bearer_token_required:
                return "admin bearer token required";
            case http_error_code::superadmin_bearer_token_required:
                return "superadmin bearer token required";
            case http_error_code::invalid_credentials:
                return "invalid credentials";
            case http_error_code::submission_banned:
                return "submission is currently banned";
            case http_error_code::invalid_testcase_zip:
                return "invalid testcase zip";
        }
        return "unknown http error";
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
            ? default_message(code_value)
            : std::move(message_value)
    ),
    field_opt(std::move(field_opt_value)){}

http_error::http_error(const service_error& error)
:
    http_error(from_service_error(error)){}

boost::beast::http::status http_error::status() const{
    return to_status_value(code);
}

std::string_view http_error::code_string() const{
    return to_code_string(code);
}

bool http_error::requires_bearer_auth() const{
    return requires_bearer_auth_value(code);
}

http_error http_error::from_service_error(const service_error& error){
    return http_error{map_service_error_code(error)};
}

namespace{
    boost::beast::http::status to_status_value(http_error_code ec){
        using status = boost::beast::http::status;

        switch(ec){
            case http_error_code::bad_request:
            case http_error_code::validation_error:
            case http_error_code::invalid_json:
            case http_error_code::invalid_query_string:
            case http_error_code::duplicate_query_parameter:
            case http_error_code::invalid_query_parameter:
            case http_error_code::unsupported_query_parameter:
            case http_error_code::missing_field:
            case http_error_code::invalid_field:
            case http_error_code::invalid_length:
            case http_error_code::invalid_argument:
            case http_error_code::invalid_testcase_zip:
                return status::bad_request;
            case http_error_code::payload_too_large:
                return status::payload_too_large;
            case http_error_code::method_not_allowed:
                return status::method_not_allowed;
            case http_error_code::unauthorized:
            case http_error_code::missing_or_invalid_bearer_token:
            case http_error_code::invalid_or_expired_token:
            case http_error_code::admin_bearer_token_required:
            case http_error_code::superadmin_bearer_token_required:
            case http_error_code::invalid_credentials:
                return status::unauthorized;
            case http_error_code::forbidden:
            case http_error_code::submission_banned:
                return status::forbidden;
            case http_error_code::not_found:
                return status::not_found;
            case http_error_code::conflict:
                return status::conflict;
            case http_error_code::service_unavailable:
                return status::service_unavailable;
            case http_error_code::internal_server_error:
                return status::internal_server_error;
        }

        return status::internal_server_error;
    }
}

std::string_view to_code_string(http_error_code ec){
    switch(ec){
        case http_error_code::bad_request:
            return "bad_request";
        case http_error_code::payload_too_large:
            return "payload_too_large";
        case http_error_code::method_not_allowed:
            return "method_not_allowed";
        case http_error_code::validation_error:
            return "validation_error";
        case http_error_code::unauthorized:
            return "unauthorized";
        case http_error_code::forbidden:
            return "forbidden";
        case http_error_code::not_found:
            return "not_found";
        case http_error_code::conflict:
            return "conflict";
        case http_error_code::service_unavailable:
            return "service_unavailable";
        case http_error_code::internal_server_error:
            return "internal_server_error";
        case http_error_code::invalid_json:
            return "invalid_json";
        case http_error_code::invalid_query_string:
            return "invalid_query_string";
        case http_error_code::duplicate_query_parameter:
            return "duplicate_query_parameter";
        case http_error_code::invalid_query_parameter:
            return "invalid_query_parameter";
        case http_error_code::unsupported_query_parameter:
            return "unsupported_query_parameter";
        case http_error_code::missing_field:
            return "missing_field";
        case http_error_code::invalid_field:
            return "invalid_field";
        case http_error_code::invalid_length:
            return "invalid_length";
        case http_error_code::invalid_argument:
            return "invalid_argument";
        case http_error_code::missing_or_invalid_bearer_token:
            return "missing_or_invalid_bearer_token";
        case http_error_code::invalid_or_expired_token:
            return "invalid_or_expired_token";
        case http_error_code::admin_bearer_token_required:
            return "admin_bearer_token_required";
        case http_error_code::superadmin_bearer_token_required:
            return "superadmin_bearer_token_required";
        case http_error_code::invalid_credentials:
            return "invalid_credentials";
        case http_error_code::submission_banned:
            return "submission_banned";
        case http_error_code::invalid_testcase_zip:
            return "invalid_testcase_zip";
    }

    return "unknown_http_error";
}

namespace{
    bool requires_bearer_auth_value(http_error_code ec){
        switch(ec){
            case http_error_code::missing_or_invalid_bearer_token:
            case http_error_code::invalid_or_expired_token:
            case http_error_code::admin_bearer_token_required:
            case http_error_code::superadmin_bearer_token_required:
                return true;
            default:
                return false;
        }
    }
}
