#pragma once

#include "common/db_connection.hpp"
#include "error/service_error.hpp"
#include "common/submission_status.hpp"
#include "dto/user_statistics_dto.hpp"

#include <cstdint>
#include <expected>
#include <optional>

namespace user_statistics_service{
    std::expected<user_statistics_dto::submission_statistics, service_error> get_submission_statistics(
        db_connection& connection,
        std::int64_t user_id
    );

    std::expected<void, service_error> create_user_submission_statistics(
        db_connection& connection,
        std::int64_t user_id
    );

    std::expected<void, service_error> ensure_user_submission_statistics(
        db_connection& connection,
        std::int64_t user_id
    );

    std::expected<void, service_error> record_submission_created(
        db_connection& connection,
        std::int64_t user_id
    );

    std::expected<void, service_error> record_submission_status_transition(
        db_connection& connection,
        std::int64_t user_id,
        std::optional<submission_status> from_status_opt,
        submission_status to_status
    );
}
