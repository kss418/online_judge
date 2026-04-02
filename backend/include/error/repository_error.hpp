#pragma once

#include <string>

struct error_code;

enum class repository_error_code{
    invalid_reference,
    invalid_input,
    not_found,
    conflict,
    internal
};

struct repository_error{
    repository_error_code code;
    std::string message;

    repository_error(repository_error_code code_value, std::string message_value = {});

    bool operator==(const repository_error& other) const;

    static const repository_error invalid_reference;
    static const repository_error invalid_input;
    static const repository_error not_found;
    static const repository_error conflict;
    static const repository_error internal;

    static repository_error from_error_code(const error_code& ec);
};

std::string to_string(repository_error_code ec);
std::string to_string(const repository_error& ec);
