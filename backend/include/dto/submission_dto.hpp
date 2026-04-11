#pragma once

#include "common/submission_status.hpp"
#include "dto/submission_domain_dto.hpp"
#include "dto/submission_internal_dto.hpp"
#include "dto/submission_request_dto.hpp"
#include "dto/submission_response_dto.hpp"

#include <optional>
#include <string_view>

namespace submission_dto{
    inline constexpr auto DEFAULT_LIST_LIMIT = submission_request_dto::DEFAULT_LIST_LIMIT;

    using history = submission_response_dto::history;
    using source = submission_request_dto::submit_request;
    using source_detail = submission_response_dto::source_detail;
    using create_request = submission_internal_dto::create_submission_command;
    using queued_response = submission_response_dto::queued_response;
    using summary = submission_response_dto::summary;
    using detail = submission_response_dto::detail;
    using status_snapshot = submission_response_dto::status_snapshot;
    using status_batch_request = submission_request_dto::status_batch_request;
    using summary_page = submission_response_dto::summary_page;
    using list_filter = submission_request_dto::list_filter;
    using leased_submission = submission_domain_dto::leased_submission;
    using lease_request = submission_internal_dto::lease_request;
    using status_update = submission_internal_dto::status_update;
    using finalize_request = submission_internal_dto::finalize_request;
    using finalize_result = submission_internal_dto::finalize_result;
    using history_list = submission_response_dto::history_list;

    bool is_valid(const source& source_value);
    bool is_valid(const create_request& create_request_value);
    bool is_valid(const lease_request& lease_request_value);
    bool is_valid(const status_update& status_update_value);
    bool is_valid(const finalize_request& finalize_request_value);
    queued_response make_queued_response(
        std::int64_t submission_id,
        submission_status submission_status_value,
        std::int32_t problem_version
    );
    std::optional<submission_status> make_submission_status(std::string_view submission_status_string);
    status_update make_status_update(
        const leased_submission& leased_submission_value,
        submission_status to_status,
        std::optional<std::string> reason_opt = std::nullopt
    );
    finalize_request make_finalize_request(
        const leased_submission& leased_submission_value,
        submission_status to_status,
        std::optional<std::int16_t> score_opt = std::nullopt,
        std::optional<std::string> compile_output_opt = std::nullopt,
        std::optional<std::string> judge_output_opt = std::nullopt,
        std::optional<std::int64_t> elapsed_ms_opt = std::nullopt,
        std::optional<std::int64_t> max_rss_kb_opt = std::nullopt,
        std::optional<std::string> reason_opt = std::nullopt
    );
    finalize_result make_finalize_result(
        std::int64_t problem_id,
        bool should_increase_accepted_count
    );
}
