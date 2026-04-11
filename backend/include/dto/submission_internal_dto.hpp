#pragma once

#include "common/submission_status.hpp"
#include "dto/submission_domain_dto.hpp"
#include "dto/submission_request_dto.hpp"

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>

namespace submission_internal_dto{
    struct create_submission_command{
        std::int64_t user_id = 0;
        std::int64_t problem_id = 0;
        submission_request_dto::submit_request source_value;
    };

    struct lease_request{
        std::chrono::seconds lease_duration{0};
    };

    struct status_update{
        std::int64_t submission_id = 0;
        std::int32_t attempt_no = 0;
        std::string lease_token;
        submission_status to_status = submission_status::queued;
        std::optional<std::string> reason_opt = std::nullopt;
    };

    struct finalize_request{
        std::int64_t submission_id = 0;
        std::int32_t attempt_no = 0;
        std::string lease_token;
        submission_status to_status = submission_status::queued;
        std::optional<std::int16_t> score_opt = std::nullopt;
        std::optional<std::string> compile_output_opt = std::nullopt;
        std::optional<std::string> judge_output_opt = std::nullopt;
        std::optional<std::int64_t> elapsed_ms_opt = std::nullopt;
        std::optional<std::int64_t> max_rss_kb_opt = std::nullopt;
        std::optional<std::string> reason_opt = std::nullopt;
    };

    struct finalize_result{
        std::int64_t problem_id = 0;
        bool should_increase_accepted_count = false;
    };

    bool is_valid(const create_submission_command& command_value);
    bool is_valid(const lease_request& lease_request_value);
    bool is_valid(const status_update& status_update_value);
    bool is_valid(const finalize_request& finalize_request_value);
    status_update make_status_update(
        const submission_domain_dto::leased_submission& leased_submission_value,
        submission_status to_status,
        std::optional<std::string> reason_opt = std::nullopt
    );
    finalize_request make_finalize_request(
        const submission_domain_dto::leased_submission& leased_submission_value,
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
