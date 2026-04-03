#include "error/http_server_error.hpp"

#include "error/error_code.hpp"
#include "error/infra_error.hpp"
#include "error/pool_error.hpp"

#include <array>
#include <cstddef>
#include <string_view>
#include <utility>

namespace{
    struct http_server_error_spec{
        std::string_view default_message;
    };

    constexpr http_server_error_spec unknown_http_server_error_spec{
        "unknown http server error"
    };

    constexpr std::array<http_server_error_spec, 3> http_server_error_specs{{
        {"invalid http server configuration"},
        {"http server unavailable"},
        {"http server internal error"},
    }};

    const http_server_error_spec& describe_http_server_error(
        http_server_error_code code
    ){
        const auto index = static_cast<std::size_t>(code);
        if(index >= http_server_error_specs.size()){
            return unknown_http_server_error_spec;
        }

        return http_server_error_specs[index];
    }

    template<typename... Candidates>
    constexpr bool matches_any(const error_code& error, Candidates... candidates){
        return ((error == candidates) || ...);
    }

    http_server_error_code map_infra_error_code(const infra_error& error){
        switch(error.code){
            case infra_error_code::invalid_argument:
            case infra_error_code::permission_denied:
            case infra_error_code::not_found:
            case infra_error_code::conflict:
                return http_server_error_code::invalid_configuration;
            case infra_error_code::unavailable:
                return http_server_error_code::unavailable;
            case infra_error_code::internal:
                return http_server_error_code::internal;
        }

        return http_server_error_code::internal;
    }

    http_server_error_code map_pool_error_code(const pool_error& error){
        switch(error.code){
            case pool_error_code::invalid_argument:
                return http_server_error_code::invalid_configuration;
            case pool_error_code::timed_out:
            case pool_error_code::unavailable:
                return http_server_error_code::unavailable;
            case pool_error_code::internal:
                return http_server_error_code::internal;
        }

        return http_server_error_code::internal;
    }

    http_server_error_code map_error_code_kind(const error_code& error){
        if(matches_any(
            error,
            errno_error::invalid_argument,
            errno_error::permission_denied,
            errno_error::file_not_found,
            errno_error::file_exists
        )){
            return http_server_error_code::invalid_configuration;
        }

        if(matches_any(
            error,
            errno_error::resource_temporarily_unavailable,
            boost_error::timed_out,
            boost_error::would_block,
            boost_error::try_again
        )){
            return http_server_error_code::unavailable;
        }

        return http_server_error_code::internal;
    }
}

http_server_error::http_server_error(
    http_server_error_code code_value,
    std::string message_value
):
    code(code_value),
    message(
        message_value.empty()
            ? std::string{describe_http_server_error(code_value).default_message}
            : std::move(message_value)
    ){}

http_server_error::http_server_error(const infra_error& error)
:
    http_server_error(map_infra_error_code(error), error.message){}

http_server_error::http_server_error(const pool_error& error)
:
    http_server_error(map_pool_error_code(error), error.message){}

http_server_error::http_server_error(const error_code& error)
:
    http_server_error(map_error_code_kind(error), to_string(error)){}

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
    return std::string{describe_http_server_error(error).default_message};
}

std::string to_string(const http_server_error& error){
    return error.message;
}
