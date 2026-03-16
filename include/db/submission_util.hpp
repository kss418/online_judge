#pragma once
#include "common/error_code.hpp"

#include <chrono>
#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <string_view>

namespace pqxx{
class transaction_base;
}

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

std::string to_string(submission_status status);

struct queued_submission{
    std::int64_t submission_id = 0;
    std::int64_t problem_id = 0;
    std::string language;
    std::string source_code;
};

namespace submission_util{
    inline constexpr std::string_view SUBMISSION_QUEUE_CHANNEL = "submission_queue";

    std::expected<std::int64_t, error_code> create_submission(
        pqxx::transaction_base& transaction,
        std::int64_t user_id,
        std::int64_t problem_id,
        const std::string& language,
        const std::string& source_code
    );

    std::expected<void, error_code> update_submission_status(
        pqxx::transaction_base& transaction,
        std::int64_t submission_id,
        submission_status to_status,
        const std::optional<std::string>& reason = std::nullopt
    );

    std::expected<queued_submission, error_code> lease_submission(
        pqxx::transaction_base& transaction,
        std::chrono::seconds lease_duration
    );

    std::expected<void, error_code> finalize_submission(
        pqxx::transaction_base& transaction,
        std::int64_t submission_id,
        submission_status to_status,
        std::optional<std::int16_t> score,
        std::optional<std::string> compile_output,
        std::optional<std::string> judge_output,
        std::optional<std::string> reason
    );
}
