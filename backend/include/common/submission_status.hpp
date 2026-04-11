#pragma once

#include <cstdint>
#include <optional>
#include <span>
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
    build_resource_exceeded,
    output_exceeded,
    infra_failure
};

inline constexpr std::string_view SUBMISSION_QUEUE_CHANNEL = "submission_queue";

struct submission_status_spec{
    submission_status status;
    std::string_view code;
    std::string_view db_enum_value;
    std::string_view public_label_ko;
    std::string_view badge_tone;
    std::string_view statistics_bucket;
    bool is_terminal = false;
    bool is_failure = false;
    bool filter_visible = false;
    bool statistics_visible = false;
};

std::string to_string(submission_status status);
std::optional<submission_status> parse_submission_status(std::string_view status);
std::span<const submission_status_spec> all_submission_status_specs();
const submission_status_spec* find_submission_status_spec(submission_status status);
