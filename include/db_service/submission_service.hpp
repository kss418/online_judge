#pragma once

#include "common/error_code.hpp"
#include "common/db_connection.hpp"
#include "common/submission_status.hpp"
#include "dto/submission_dto.hpp"

#include <chrono>
#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <vector>

namespace submission_service{
    std::expected<submission_dto::history_list, error_code> get_submission_history(
        db_connection& connection,
        std::int64_t submission_id
    );
    std::expected<submission_dto::source_detail, error_code> get_submission_source(
        db_connection& connection,
        std::int64_t submission_id
    );
    std::expected<submission_dto::detail, error_code> get_submission_detail(
        db_connection& connection,
        std::int64_t submission_id
    );
    std::expected<submission_dto::created, error_code> create_submission(
        db_connection& connection,
        const submission_dto::create_request& create_request_value
    );
    std::expected<std::vector<submission_dto::summary>, error_code> list_submissions(
        db_connection& connection,
        const submission_dto::list_filter& filter_value
    );
    std::expected<void, error_code> update_submission_status(
        db_connection& connection,
        const submission_dto::status_update& status_update_value
    );
    std::expected<std::optional<submission_dto::queued_submission>, error_code> lease_submission(
        db_connection& connection,
        const submission_dto::lease_request& lease_request_value
    );
    std::expected<void, error_code> finalize_submission(
        db_connection& connection,
        const submission_dto::finalize_request& finalize_request_value
    );
}
