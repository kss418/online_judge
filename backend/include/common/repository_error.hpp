#pragma once

#include <optional>
#include <string>

struct error_code;

enum class repository_error{
    invalid_reference,
    invalid_input,
    not_found,
    conflict,
    internal
};

std::string to_string(repository_error ec);
std::optional<repository_error> map_error_to_repository_error(const error_code& ec);
