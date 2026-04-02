#pragma once

#include "common/query_param.hpp"
#include "common/submission_status.hpp"
#include "dto/dto_validation_error.hpp"

#include <boost/json/fwd.hpp>

#include <cstddef>
#include <chrono>
#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <vector>

namespace submission_dto{
    inline constexpr std::int32_t DEFAULT_LIST_LIMIT = 50;

    struct history{
        std::int64_t history_id = 0;
        std::optional<std::string> from_status_opt = std::nullopt;
        std::string to_status;
        std::optional<std::string> reason_opt = std::nullopt;
        std::string created_at;
    };

    struct source{
        std::string language;
        std::string source_code;
    };

    struct source_detail{
        std::int64_t submission_id = 0;
        std::int64_t user_id = 0;
        std::int64_t problem_id = 0;
        std::string language;
        std::string source_code;
        std::optional<std::string> compile_output_opt = std::nullopt;
        std::optional<std::string> judge_output_opt = std::nullopt;
    };

    struct create_request{
        std::int64_t user_id = 0;
        std::int64_t problem_id = 0;
        source source_value;
    };

    struct created{
        std::int64_t submission_id = 0;
        std::string status;
    };

    struct summary{
        std::int64_t submission_id = 0;
        std::int64_t user_id = 0;
        std::string user_login_id;
        std::int64_t problem_id = 0;
        std::string problem_title;
        std::string language;
        std::string status;
        std::optional<std::int16_t> score_opt = std::nullopt;
        std::optional<std::int64_t> elapsed_ms_opt = std::nullopt;
        std::optional<std::int64_t> max_rss_kb_opt = std::nullopt;
        std::optional<std::string> user_problem_state_opt = std::nullopt;
        std::string created_at;
        std::string updated_at;
    };

    struct detail{
        std::int64_t submission_id = 0;
        std::int64_t user_id = 0;
        std::int64_t problem_id = 0;
        std::string language;
        std::string status;
        std::optional<std::int16_t> score_opt = std::nullopt;
        std::optional<std::string> compile_output_opt = std::nullopt;
        std::optional<std::string> judge_output_opt = std::nullopt;
        std::optional<std::int64_t> elapsed_ms_opt = std::nullopt;
        std::optional<std::int64_t> max_rss_kb_opt = std::nullopt;
        std::string created_at;
        std::string updated_at;
    };

    struct status_snapshot{
        std::int64_t submission_id = 0;
        std::string status;
        std::optional<std::int16_t> score_opt = std::nullopt;
        std::optional<std::int64_t> elapsed_ms_opt = std::nullopt;
        std::optional<std::int64_t> max_rss_kb_opt = std::nullopt;
    };

    struct status_batch_request{
        std::vector<std::int64_t> submission_ids;
    };

    struct summary_page{
        std::vector<summary> submissions;
        bool has_more = false;
        std::optional<std::int64_t> next_before_submission_id_opt = std::nullopt;
    };

    struct list_filter{
        std::optional<std::int64_t> user_id_opt = std::nullopt;
        std::optional<std::string> user_login_id_opt = std::nullopt;
        std::optional<std::int64_t> problem_id_opt = std::nullopt;
        std::optional<std::string> language_opt = std::nullopt;
        std::optional<std::string> status_opt = std::nullopt;
        std::optional<std::int32_t> limit_opt = std::nullopt;
        std::optional<std::int64_t> before_submission_id_opt = std::nullopt;
    };

    struct queued_submission{
        std::int64_t submission_id = 0;
        std::int64_t problem_id = 0;
        std::int64_t queue_wait_ms = 0;
        std::string language;
        std::string source_code;
    };

    struct lease_request{
        std::chrono::seconds lease_duration{0};
    };

    struct status_update{
        std::int64_t submission_id = 0;
        submission_status to_status = submission_status::queued;
        std::optional<std::string> reason_opt = std::nullopt;
    };

    struct finalize_request{
        std::int64_t submission_id = 0;
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

    using history_list = std::vector<history>;

    bool is_valid(const source& source_value);
    bool is_valid(const create_request& create_request_value);
    bool is_valid(const lease_request& lease_request_value);
    bool is_valid(const status_update& status_update_value);
    bool is_valid(const finalize_request& finalize_request_value);

    std::expected<source, dto_validation_error> make_source_from_json(
        const boost::json::object& json
    );
    std::expected<status_batch_request, dto_validation_error> make_status_batch_request_from_json(
        const boost::json::object& json
    );
    std::expected<list_filter, dto_validation_error> make_list_filter_from_query_params(
        const std::vector<query_param>& query_params
    );
    std::expected<create_request, dto_validation_error> make_create_request_from_json(
        const boost::json::object& json,
        std::int64_t user_id,
        std::int64_t problem_id
    );
    created make_created(
        std::int64_t submission_id,
        submission_status submission_status_value
    );
    std::optional<submission_status> make_submission_status(std::string_view submission_status_string);
    status_update make_status_update(
        std::int64_t submission_id,
        submission_status to_status,
        std::optional<std::string> reason_opt = std::nullopt
    );
    finalize_request make_finalize_request(
        std::int64_t submission_id,
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
