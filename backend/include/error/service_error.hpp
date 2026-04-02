#pragma once

#include "error/repository_error.hpp"

#include <optional>
#include <string>

struct error_code;

enum class service_error{
    validation_error,
    unauthorized,
    forbidden,
    not_found,
    conflict,
    unavailable,
    internal
};

std::string to_string(service_error ec);

namespace service_error_util{
    std::optional<service_error> from_repository(repository_error ec);
    std::optional<service_error> from_error_code(const error_code& ec);
}
