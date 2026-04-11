#pragma once

#include "common/submission_status.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace submission_response_dto{
    struct history{
        std::int64_t history_id = 0;
        std::optional<std::string> from_status_opt = std::nullopt;
        std::string to_status;
        std::optional<std::string> reason_opt = std::nullopt;
        std::string created_at;
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

    struct queued_response{
        std::int64_t submission_id = 0;
        std::string status;
        std::int32_t problem_version = 0;
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

    using history_list = std::vector<history>;

    struct summary_page{
        std::vector<summary> submissions;
        bool has_more = false;
        std::optional<std::int64_t> next_before_submission_id_opt = std::nullopt;
    };

    queued_response make_queued_response(
        std::int64_t submission_id,
        submission_status submission_status_value,
        std::int32_t problem_version
    );
}
