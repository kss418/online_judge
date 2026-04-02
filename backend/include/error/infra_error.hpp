#pragma once

#include <string>

struct error_code;

enum class infra_error_code{
    invalid_argument,
    permission_denied,
    not_found,
    conflict,
    unavailable,
    internal
};

struct infra_error{
    infra_error_code code;
    std::string message;

    infra_error(infra_error_code code_value, std::string message_value = {});

    bool operator==(const infra_error& other) const;

    bool is_transient() const;

    static const infra_error invalid_argument;
    static const infra_error permission_denied;
    static const infra_error not_found;
    static const infra_error conflict;
    static const infra_error unavailable;
    static const infra_error internal;

    static infra_error from_error_code(const error_code& ec);
};

std::string to_string(infra_error_code ec);
std::string to_string(const infra_error& ec);
