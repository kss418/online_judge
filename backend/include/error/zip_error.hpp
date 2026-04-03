#pragma once

#include <string>
#include <system_error>

struct io_error;

enum class zip_error_code{
    invalid_argument,
    command_unavailable,
    invalid_archive,
    unavailable,
    internal
};

struct zip_error{
    zip_error_code code;
    std::string message;

    zip_error(zip_error_code code_value, std::string message_value = {});
    zip_error(const io_error& error);

    bool operator==(const zip_error& other) const;

    static zip_error from_io_error(const io_error& error);
    static zip_error from_errno(int error_number);
    static zip_error from_error_code(const std::error_code& error);

    static const zip_error invalid_argument;
    static const zip_error command_unavailable;
    static const zip_error invalid_archive;
    static const zip_error unavailable;
    static const zip_error internal;
};

std::string to_string(zip_error_code error);
std::string to_string(const zip_error& error);
