#pragma once

#include "error/http_error.hpp"

#include <optional>
#include <string>
#include <string_view>

enum class request_error_code{
    bad_request,
    payload_too_large,
    invalid_json,
    invalid_query_string,
    duplicate_query_parameter,
    invalid_query_parameter,
    unsupported_query_parameter,
    missing_field,
    invalid_field,
    invalid_length,
    invalid_argument
};

namespace request_error{
    http_error make_error(
        request_error_code code,
        std::string message = {},
        std::optional<std::string> field_opt = std::nullopt
    );

    http_error make_bad_request_error(std::string message = {});
    http_error make_payload_too_large_error();
    http_error make_invalid_json_error();
    http_error make_invalid_query_string_error();
    http_error make_duplicate_query_parameter_error(std::string_view key);
    http_error make_invalid_query_parameter_error(
        std::string_view key,
        std::string message = {}
    );
    http_error make_unsupported_query_parameter_error(std::string_view key);
    http_error make_missing_field_error(std::string_view field_name);
    http_error make_invalid_field_error(
        std::string_view field_name,
        std::string message = {}
    );
    http_error make_invalid_length_error(
        std::string_view field_name,
        std::string message = {}
    );
    http_error make_invalid_argument_error(
        std::string message = {},
        std::optional<std::string> field_opt = std::nullopt
    );
}
