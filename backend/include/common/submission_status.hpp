#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

enum class submission_status{
    queued,
    judging,
    accepted,
    wrong_answer,
    time_limit_exceeded,
    memory_limit_exceeded,
    runtime_error,
    compile_error,
    output_exceeded
};

inline constexpr std::string_view SUBMISSION_QUEUE_CHANNEL = "submission_queue";

std::string to_string(submission_status status);
std::optional<submission_status> parse_submission_status(std::string_view status);
