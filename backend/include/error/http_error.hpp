#pragma once

#include "error/service_error.hpp"

#include <optional>
#include <string>

struct error_code;

enum class http_error{
    validation_error,
    unauthorized,
    forbidden,
    not_found,
    conflict,
    internal
};

std::string to_string(http_error ec);
std::optional<http_error> from_service(service_error ec);
std::optional<http_error> from_error_code(const error_code& ec);
