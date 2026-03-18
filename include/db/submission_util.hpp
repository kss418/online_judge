#pragma once
#include "common/error_code.hpp"
#include "dto/submission_dto.hpp"

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

namespace submission_util{
    inline constexpr std::string_view SUBMISSION_QUEUE_CHANNEL = "submission_queue";

    std::expected<std::int64_t, error_code> create_submission(
        pqxx::transaction_base& transaction,
        std::int64_t user_id,
        std::int64_t problem_id,
        const submission_dto::source& source_value
    );

    std::expected<void, error_code> update_submission_status(
        pqxx::transaction_base& transaction,
        std::int64_t submission_id,
        submission_status to_status,
        const std::optional<std::string>& reason_opt = std::nullopt
    );

    std::expected<submission_dto::queued_submission, error_code> lease_submission(
        pqxx::transaction_base& transaction,
        std::chrono::seconds lease_duration
    );

    std::expected<void, error_code> finalize_submission(
        pqxx::transaction_base& transaction,
        std::int64_t submission_id,
        submission_status to_status,
        std::optional<std::int16_t> score_opt,
        std::optional<std::string> compile_output_opt,
        std::optional<std::string> judge_output_opt,
        std::optional<std::string> reason_opt
    );
}
