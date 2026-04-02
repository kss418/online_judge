#pragma once

#include "common/repository_error.hpp"
#include "common/submission_status.hpp"

#include <cstdint>
#include <expected>

namespace pqxx{
    class transaction_base;
}

namespace user_problem_summary_repository{
    std::expected<void, repository_error> upsert_summary_counts(
        pqxx::transaction_base& transaction,
        std::int64_t user_id,
        std::int64_t problem_id,
        std::int64_t submission_count,
        std::int64_t accepted_submission_count,
        std::int64_t failed_submission_count
    );

    std::expected<void, repository_error> increase_submission_count(
        pqxx::transaction_base& transaction,
        std::int64_t user_id,
        std::int64_t problem_id
    );

    std::expected<void, repository_error> increase_accepted_submission_count(
        pqxx::transaction_base& transaction,
        std::int64_t user_id,
        std::int64_t problem_id
    );

    std::expected<void, repository_error> decrease_accepted_submission_count(
        pqxx::transaction_base& transaction,
        std::int64_t user_id,
        std::int64_t problem_id
    );

    std::expected<void, repository_error> increase_failed_submission_count(
        pqxx::transaction_base& transaction,
        std::int64_t user_id,
        std::int64_t problem_id
    );

    std::expected<void, repository_error> decrease_failed_submission_count(
        pqxx::transaction_base& transaction,
        std::int64_t user_id,
        std::int64_t problem_id
    );

    std::expected<void, repository_error> apply_submission_status_transition(
        pqxx::transaction_base& transaction,
        std::int64_t user_id,
        std::int64_t problem_id,
        submission_status from_status,
        submission_status to_status
    );
}
