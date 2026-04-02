#pragma once

#include "error/repository_error.hpp"
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
    std::expected<user_statistics_dto::submission_statistics, repository_error> get_submission_statistics(
        pqxx::transaction_base& transaction,
        std::int64_t user_id
    );

    std::expected<void, repository_error> touch_timestamp_column(
        pqxx::transaction_base& transaction,
        std::int64_t user_id,
        std::string_view column_name
    );

    std::expected<void, repository_error> create_user_submission_statistics(
        pqxx::transaction_base& transaction,
        std::int64_t user_id
    );

    std::expected<void, repository_error> ensure_user_submission_statistics(
        pqxx::transaction_base& transaction,
        std::int64_t user_id
    );

    std::expected<void, repository_error> increase_submission_count(
        pqxx::transaction_base& transaction,
        std::int64_t user_id
    );

    std::expected<void, repository_error> increase_status_count(
        pqxx::transaction_base& transaction,
        std::int64_t user_id,
        submission_status status
    );

    std::expected<void, repository_error> decrease_status_count(
        pqxx::transaction_base& transaction,
        std::int64_t user_id,
        submission_status status
    );

    std::expected<void, repository_error> touch_last_submission_at(
        pqxx::transaction_base& transaction,
        std::int64_t user_id
    );

    std::expected<void, repository_error> touch_last_accepted_at(
        pqxx::transaction_base& transaction,
        std::int64_t user_id
    );
}
