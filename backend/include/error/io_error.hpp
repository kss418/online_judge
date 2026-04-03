#pragma once

#include <string>
#include <system_error>

enum class io_error_code{
    invalid_argument,
    permission_denied,
    not_found,
    conflict,
    unavailable,
    internal
};

struct io_error{
    io_error_code code;
    std::string message;
    bool retryable = false;

    io_error(
        io_error_code code_value,
        std::string message_value = {},
        bool retryable_value = false
    );

    bool operator==(const io_error& other) const;

    bool is_retryable() const;

    static io_error from_errno(int error_number);
    static io_error from_error_code(const std::error_code& error);

    static const io_error invalid_argument;
    static const io_error permission_denied;
    static const io_error not_found;
    static const io_error conflict;
    static const io_error unavailable;
    static const io_error internal;
};

std::string to_string(io_error_code code);
std::string to_string(const io_error& error);
