#pragma once

#include "error/repository_error.hpp"

#include <string>

struct infra_error;

enum class service_error_code{
    validation_error,
    unauthorized,
    forbidden,
    not_found,
    conflict,
    unavailable,
    internal
};

struct service_error{
    service_error_code code;
    std::string message;

    service_error(service_error_code code_value, std::string message_value = {});
    service_error(const repository_error& ec);
    service_error(const infra_error& ec);

    bool operator==(const service_error& other) const;

    static const service_error validation_error;
    static const service_error unauthorized;
    static const service_error forbidden;
    static const service_error not_found;
    static const service_error conflict;
    static const service_error unavailable;
    static const service_error internal;

    static service_error from_infra_error(const infra_error& ec);
    static service_error from_repository(const repository_error& ec);
};

std::string to_string(service_error_code ec);
std::string to_string(const service_error& ec);
