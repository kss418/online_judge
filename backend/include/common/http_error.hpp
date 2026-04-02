#pragma once

#include <optional>
#include <string>

struct error_code;

enum class http_error{
    validation_error,
    unauthorized,
    forbidden,
    not_found,
    conflict
};

std::string to_string(http_error ec);
std::optional<http_error> map_error_to_http_error(const error_code& ec);
