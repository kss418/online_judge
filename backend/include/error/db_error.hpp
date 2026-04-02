#pragma once

#include <exception>
#include <string>

enum class db_error_code{
    invalid_argument,
    invalid_connection,
    interrupted,
    unique_violation,
    constraint_violation,
    broken_connection,
    serialization_failure,
    deadlock_detected,
    unavailable,
    internal
};

struct db_error{
    db_error_code code;
    std::string message;

    db_error(db_error_code code_value, std::string message_value = {});

    bool operator==(const db_error& other) const;

    bool is_retryable() const;
    bool should_reconnect() const;
    bool is_constraint_violation() const;

    static const db_error invalid_argument;
    static const db_error invalid_connection;
    static const db_error interrupted;
    static const db_error unique_violation;
    static const db_error constraint_violation;
    static const db_error broken_connection;
    static const db_error serialization_failure;
    static const db_error deadlock_detected;
    static const db_error unavailable;
    static const db_error internal;

    static db_error from_psql_exception(const std::exception& exception);
};

std::string to_string(db_error_code ec);
std::string to_string(const db_error& ec);
