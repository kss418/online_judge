#pragma once

#include "common/db_connection.hpp"
#include "db_event/submission_event_listener.hpp"
#include "dto/submission_dto.hpp"
#include "error/judge_error.hpp"

#include <chrono>
#include <expected>
#include <optional>

namespace judge_service{
    std::expected<std::optional<submission_dto::queued_submission>, judge_error>
    poll_next_submission(
        db_connection& connection,
        submission_event_listener& submission_event_listener,
        std::chrono::seconds lease_duration,
        std::chrono::milliseconds notification_wait_timeout
    );
}
