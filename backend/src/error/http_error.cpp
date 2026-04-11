#include "error/http_error.hpp"

#include <utility>

namespace{
    const http_error_spec unknown_http_error_spec{
        http_error_code::internal_server_error,
        "unknown_http_error",
        boost::beast::http::status::internal_server_error,
        "unknown http error",
        "",
        "",
        false,
        false,
        "server"
    };

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
            ? std::string{
                (find_http_error_spec(code_value) != nullptr)
                    ? find_http_error_spec(code_value)->default_public_message_en
                    : unknown_http_error_spec.default_public_message_en
            }
            : std::move(message_value)
    ),
    field_opt(std::move(field_opt_value)){}

http_error::http_error(const service_error& error)
:
    http_error(from_service_error(error)){}

boost::beast::http::status http_error::status() const{
    const auto* spec = find_http_error_spec(code);
    if(spec != nullptr){
        return spec->http_status;
    }

    return unknown_http_error_spec.http_status;
}

std::string_view http_error::code_string() const{
    const auto* spec = find_http_error_spec(code);
    if(spec != nullptr){
        return spec->code_string;
    }

    return unknown_http_error_spec.code_string;
}

bool http_error::requires_bearer_auth() const{
    const auto* spec = find_http_error_spec(code);
    if(spec != nullptr){
        return spec->requires_bearer_auth;
    }

    return unknown_http_error_spec.requires_bearer_auth;
}

http_error http_error::from_service_error(const service_error& error){
    return http_error{map_service_error_code(error)};
}

std::string_view to_code_string(http_error_code ec){
    const auto* spec = find_http_error_spec(ec);
    if(spec != nullptr){
        return spec->code_string;
    }

    return unknown_http_error_spec.code_string;
}
