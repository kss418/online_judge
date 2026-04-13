#pragma once

#include "common/db_connection.hpp"
#include "dto/submission_request_dto.hpp"
#include "dto/submission_response_dto.hpp"
#include "error/service_error.hpp"

#include <cstdint>
#include <expected>
#include <optional>
#include <vector>

namespace submission_query_service{
    std::expected<submission_response_dto::history_list, service_error> get_submission_history(
        db_connection& connection,
        std::int64_t submission_id
    );
    std::expected<submission_response_dto::source_detail, service_error> get_submission_source(
        db_connection& connection,
        std::int64_t submission_id
    );
    std::expected<submission_response_dto::detail, service_error> get_submission_detail(
        db_connection& connection,
        std::int64_t submission_id
    );
    std::expected<std::vector<submission_response_dto::status_snapshot>, service_error>
    get_submission_status_snapshots(
        db_connection& connection,
        const std::vector<std::int64_t>& submission_ids
    );
    std::expected<submission_response_dto::summary_page, service_error> list_submissions(
        db_connection& connection,
        const submission_request_dto::list_filter& filter_value,
        std::optional<std::int64_t> viewer_user_id_opt = std::nullopt
    );
}
