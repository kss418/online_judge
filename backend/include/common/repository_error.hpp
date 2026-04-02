#pragma once

#include <string>

enum class repository_error{
    invalid_reference,
    invalid_input,
    not_found,
    conflict
};

std::string to_string(repository_error ec);
