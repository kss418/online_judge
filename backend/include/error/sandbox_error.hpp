#pragma once

#include <string>
#include <system_error>

struct infra_error;
struct io_error;

enum class sandbox_error_code{
    invalid_argument,
    invalid_configuration,
    unsupported,
    unavailable,
    internal
};

struct sandbox_error{
    sandbox_error_code code;
    std::string message;

    sandbox_error(sandbox_error_code code_value, std::string message_value = {});
    sandbox_error(const io_error& error);
    sandbox_error(const infra_error& error);

    bool operator==(const sandbox_error& other) const;

    static sandbox_error from_io_error(const io_error& error);
    static sandbox_error from_infra_error(const infra_error& error);
    static sandbox_error from_errno(int error_number);
    static sandbox_error from_error_code(const std::error_code& error);

    static const sandbox_error invalid_argument;
    static const sandbox_error invalid_configuration;
    static const sandbox_error unsupported;
    static const sandbox_error unavailable;
    static const sandbox_error internal;
};

std::string to_string(sandbox_error_code error);
std::string to_string(const sandbox_error& error);
