#pragma once

#include <string>

struct db_error;

enum class pool_error_code{
    invalid_argument,
    timed_out,
    unavailable,
    internal
};

struct pool_error{
    pool_error_code code;
    std::string message;

    pool_error(pool_error_code code_value, std::string message_value = {});
    pool_error(const db_error& error);

    bool operator==(const pool_error& other) const;

    static const pool_error invalid_argument;
    static const pool_error timed_out;
    static const pool_error unavailable;
    static const pool_error internal;

    static pool_error from_db_error(const db_error& error);
};

std::string to_string(pool_error_code error);
std::string to_string(const pool_error& error);
