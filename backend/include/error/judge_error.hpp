#pragma once

#include "error/service_error.hpp"

#include <string>

struct error_code;

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

    bool operator==(const judge_error& other) const;

    static const judge_error validation_error;
    static const judge_error not_found;
    static const judge_error conflict;
    static const judge_error unavailable;
    static const judge_error internal;

    static judge_error from_service(const service_error& ec);
    static judge_error from_error_code(const error_code& ec);
};

std::string to_string(judge_error_code ec);
std::string to_string(const judge_error& ec);
