#pragma once

#include "common/error_code.hpp"

#include <cstdint>
#include <expected>

namespace pqxx{
    class transaction_base;
}

namespace user_problem_summary_repository{
    std::expected<void, error_code> upsert_summary_counts(
        pqxx::transaction_base& transaction,
        std::int64_t user_id,
        std::int64_t problem_id,
        std::int64_t submission_count,
        std::int64_t accepted_submission_count,
        std::int64_t failed_submission_count
    );

    std::expected<void, error_code> increase_submission_count(
        pqxx::transaction_base& transaction,
        std::int64_t user_id,
        std::int64_t problem_id
    );

    std::expected<void, error_code> increase_accepted_submission_count(
        pqxx::transaction_base& transaction,
        std::int64_t user_id,
        std::int64_t problem_id
    );

    std::expected<void, error_code> decrease_accepted_submission_count(
        pqxx::transaction_base& transaction,
        std::int64_t user_id,
        std::int64_t problem_id
    );

    std::expected<void, error_code> increase_failed_submission_count(
        pqxx::transaction_base& transaction,
        std::int64_t user_id,
        std::int64_t problem_id
    );

    std::expected<void, error_code> decrease_failed_submission_count(
        pqxx::transaction_base& transaction,
        std::int64_t user_id,
        std::int64_t problem_id
    );
}
