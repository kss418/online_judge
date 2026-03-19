#pragma once
#include "common/error_code.hpp"
#include "common/submission_status.hpp"
#include "dto/submission_dto.hpp"

#include <chrono>
#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <vector>

namespace pqxx{
    class transaction_base;
}

namespace submission_util{
    struct finalize_result{
        std::int64_t problem_id = 0;
        bool should_increase_accepted_count = false;
    };

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

    std::expected<finalize_result, error_code> finalize_submission(
        pqxx::transaction_base& transaction,
        std::int64_t submission_id,
        submission_status to_status,
        std::optional<std::int16_t> score_opt,
        std::optional<std::string> compile_output_opt,
        std::optional<std::string> judge_output_opt,
        std::optional<std::string> reason_opt
    );

    std::expected<std::vector<submission_dto::summary>, error_code> list_submissions(
        pqxx::transaction_base& transaction,
        const submission_dto::list_filter& filter_value
    );
}
