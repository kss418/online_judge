#pragma once

#include <string>

struct db_error;

enum class submission_event_error_code{
    invalid_argument,
    unavailable,
    internal
};

struct submission_event_error{
    submission_event_error_code code;
    std::string message;

    submission_event_error(
        submission_event_error_code code_value,
        std::string message_value = {}
    );
    submission_event_error(const db_error& error);

    bool operator==(const submission_event_error& other) const;

    static submission_event_error from_db_error(const db_error& error);

    static const submission_event_error invalid_argument;
    static const submission_event_error unavailable;
    static const submission_event_error internal;
};

std::string to_string(submission_event_error_code error);
std::string to_string(const submission_event_error& error);
