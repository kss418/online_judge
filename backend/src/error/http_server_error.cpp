#include "error/http_server_error.hpp"

#include "error/error_code.hpp"
#include "error/infra_error.hpp"
#include "error/pool_error.hpp"

#include <utility>

namespace{
    std::string default_message(http_server_error_code error){
        switch(error){
            case http_server_error_code::invalid_configuration:
                return "invalid http server configuration";
            case http_server_error_code::unavailable:
                return "http server unavailable";
            case http_server_error_code::internal:
                return "http server internal error";
        }

        return "unknown http server error";
    }
}

http_server_error::http_server_error(
    http_server_error_code code_value,
    std::string message_value
):
    code(code_value),
    message(
        message_value.empty()
            ? default_message(code_value)
            : std::move(message_value)
    ){}

http_server_error::http_server_error(const infra_error& error)
:
    http_server_error(from_infra_error(error)){}

http_server_error::http_server_error(const pool_error& error)
:
    http_server_error(from_pool_error(error)){}

http_server_error::http_server_error(const error_code& error)
:
    http_server_error(from_error_code(error)){}

bool http_server_error::operator==(const http_server_error& other) const{
    return code == other.code;
}

const http_server_error http_server_error::invalid_configuration{
    http_server_error_code::invalid_configuration
};
const http_server_error http_server_error::unavailable{
    http_server_error_code::unavailable
};
const http_server_error http_server_error::internal{
    http_server_error_code::internal
};

std::string to_string(http_server_error_code error){
    switch(error){
        case http_server_error_code::invalid_configuration:
            return "invalid http server configuration";
        case http_server_error_code::unavailable:
            return "http server unavailable";
        case http_server_error_code::internal:
            return "http server internal error";
    }

    return "unknown http server error";
}

std::string to_string(const http_server_error& error){
    return error.message;
}

http_server_error http_server_error::from_infra_error(const infra_error& error){
    switch(error.code){
        case infra_error_code::invalid_argument:
        case infra_error_code::permission_denied:
        case infra_error_code::not_found:
        case infra_error_code::conflict:
            return http_server_error{
                http_server_error_code::invalid_configuration,
                error.message
            };
        case infra_error_code::unavailable:
            return http_server_error{
                http_server_error_code::unavailable,
                error.message
            };
        case infra_error_code::internal:
            return http_server_error{
                http_server_error_code::internal,
                error.message
            };
    }

    return http_server_error::internal;
}

http_server_error http_server_error::from_pool_error(const pool_error& error){
    switch(error.code){
        case pool_error_code::invalid_argument:
            return http_server_error{
                http_server_error_code::invalid_configuration,
                error.message
            };
        case pool_error_code::timed_out:
        case pool_error_code::unavailable:
            return http_server_error{
                http_server_error_code::unavailable,
                error.message
            };
        case pool_error_code::internal:
            return http_server_error{
                http_server_error_code::internal,
                error.message
            };
    }

    return http_server_error::internal;
}

http_server_error http_server_error::from_error_code(const error_code& error){
    if(
        error == errno_error::invalid_argument ||
        error == errno_error::permission_denied ||
        error == errno_error::file_not_found ||
        error == errno_error::file_exists
    ){
        return http_server_error{
            http_server_error_code::invalid_configuration,
            to_string(error)
        };
    }

    if(
        error == errno_error::resource_temporarily_unavailable ||
        error == boost_error::timed_out ||
        error == boost_error::would_block ||
        error == boost_error::try_again
    ){
        return http_server_error{
            http_server_error_code::unavailable,
            to_string(error)
        };
    }

    return http_server_error{
        http_server_error_code::internal,
        to_string(error)
    };
}
