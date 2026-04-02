#pragma once

#include "error/error_code.hpp"
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
    std::expected<std::vector<submission_dto::status_snapshot>, error_code>
    get_submission_status_snapshots(
        db_connection& connection,
        const std::vector<std::int64_t>& submission_ids
    );
    std::expected<std::vector<submission_dto::summary>, error_code> get_wa_or_ac_submissions(
        db_connection& connection,
        std::int64_t problem_id
    );
    std::expected<submission_dto::created, error_code> create_submission(
        db_connection& connection,
        const submission_dto::create_request& create_request_value
    );
    std::expected<submission_dto::summary_page, error_code> list_submissions(
        db_connection& connection,
        const submission_dto::list_filter& filter_value,
        std::optional<std::int64_t> viewer_user_id_opt = std::nullopt
    );
    std::expected<void, error_code> update_submission_status(
        db_connection& connection,
        const submission_dto::status_update& status_update_value
    );
    std::expected<void, error_code> mark_queued(
        db_connection& connection,
        std::int64_t submission_id
    );
    std::expected<void, error_code> mark_judging(
        db_connection& connection,
        std::int64_t submission_id
    );
    std::expected<void, error_code> rejudge(
        db_connection& connection,
        std::int64_t submission_id
    );
    std::expected<void, error_code> rejudge_problem(
        db_connection& connection,
        std::int64_t problem_id
    );
    std::expected<std::optional<submission_dto::queued_submission>, error_code> lease_submission(
        db_connection& connection,
        const submission_dto::lease_request& lease_request_value
    );
    std::expected<void, error_code> requeue_submission_immediately(
        db_connection& connection,
        std::int64_t submission_id,
        std::optional<std::string> reason_opt = std::nullopt
    );
    std::expected<void, error_code> finalize_submission(
        db_connection& connection,
        const submission_dto::finalize_request& finalize_request_value
    );
}
