#pragma once

#include "common/db_connection.hpp"
#include "dto/submission_dto.hpp"
#include "error/judge_error.hpp"
#include "judge_core/submission_queue_source.hpp"

#include <chrono>
#include <cstdint>
#include <expected>
#include <optional>
#include <string>

namespace judge_service{
    std::expected<std::optional<submission_dto::queued_submission>, judge_error>
    poll_next_submission(
        db_connection& connection,
        submission_queue_source& submission_queue_source,
        std::chrono::seconds lease_duration,
        std::chrono::milliseconds notification_wait_timeout
    );

    std::expected<void, judge_error> mark_judging(
        db_connection& connection,
        std::int64_t submission_id
    );

    std::expected<void, judge_error> finalize_submission(
        db_connection& connection,
        const submission_dto::finalize_request& finalize_request_value
    );

    std::expected<void, judge_error> requeue_submission_immediately(
        db_connection& connection,
        std::int64_t submission_id,
        std::optional<std::string> reason_opt = std::nullopt
    );
}
