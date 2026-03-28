#pragma once

#include "common/error_code.hpp"
#include "common/submission_status.hpp"
#include "dto/user_statistics_dto.hpp"

#include <cstdint>
#include <expected>
#include <optional>
#include <string_view>

namespace pqxx{
    class transaction_base;
}

namespace user_statistics_repository{
    std::expected<user_statistics_dto::submission_statistics, error_code> get_submission_statistics(
        pqxx::transaction_base& transaction,
        std::int64_t user_id
    );

    std::expected<void, error_code> touch_timestamp_column(
        pqxx::transaction_base& transaction,
        std::int64_t user_id,
        std::string_view column_name
    );

    std::expected<void, error_code> create_user_submission_statistics(
        pqxx::transaction_base& transaction,
        std::int64_t user_id
    );

    std::expected<void, error_code> ensure_user_submission_statistics(
        pqxx::transaction_base& transaction,
        std::int64_t user_id
    );

    std::expected<void, error_code> increase_submission_count(
        pqxx::transaction_base& transaction,
        std::int64_t user_id
    );

    std::expected<void, error_code> increase_status_count(
        pqxx::transaction_base& transaction,
        std::int64_t user_id,
        submission_status status
    );

    std::expected<void, error_code> decrease_status_count(
        pqxx::transaction_base& transaction,
        std::int64_t user_id,
        submission_status status
    );

    std::expected<void, error_code> touch_last_submission_at(
        pqxx::transaction_base& transaction,
        std::int64_t user_id
    );

    std::expected<void, error_code> touch_last_accepted_at(
        pqxx::transaction_base& transaction,
        std::int64_t user_id
    );
}
