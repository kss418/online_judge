#pragma once

#include "error/service_error.hpp"

#include <string>

struct db_error;
struct infra_error;
struct io_error;
struct sandbox_error;
struct submission_event_error;

enum class judge_error_code{
    validation_error,
    not_found,
    conflict,
    unavailable,
    internal
};

struct judge_error{
    judge_error_code code;
    std::string message;

    judge_error(judge_error_code code_value, std::string message_value = {});
    judge_error(const db_error& ec);
    judge_error(const service_error& ec);
    judge_error(const infra_error& ec);
    judge_error(const io_error& ec);
    judge_error(const sandbox_error& ec);
    judge_error(const submission_event_error& ec);

    bool operator==(const judge_error& other) const;

    static const judge_error validation_error;
    static const judge_error not_found;
    static const judge_error conflict;
    static const judge_error unavailable;
    static const judge_error internal;
};

std::string to_string(judge_error_code ec);
std::string to_string(const judge_error& ec);
