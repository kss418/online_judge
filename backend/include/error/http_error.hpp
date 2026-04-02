#pragma once

#include "error/service_error.hpp"

#include <string>

enum class http_error_code{
    validation_error,
    unauthorized,
    forbidden,
    not_found,
    conflict,
    internal
};

struct http_error{
    http_error_code code;
    std::string message;

    http_error(http_error_code code_value, std::string message_value = {});

    bool operator==(const http_error& other) const;

    static const http_error validation_error;
    static const http_error unauthorized;
    static const http_error forbidden;
    static const http_error not_found;
    static const http_error conflict;
    static const http_error internal;

    static http_error from_service(const service_error& ec);
};

std::string to_string(http_error_code ec);
std::string to_string(const http_error& ec);
