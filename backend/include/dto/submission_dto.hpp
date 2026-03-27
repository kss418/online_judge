#pragma once

#include "common/submission_status.hpp"
#include "dto/dto_validation_error.hpp"

#include <boost/json/fwd.hpp>

#include <chrono>
#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <vector>

namespace http_util{
    struct query_param;
}

namespace pqxx{
    class result;
    class row;
}

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
        std::string user_name;
        std::int64_t problem_id = 0;
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

    struct list_filter{
        std::optional<std::int32_t> page_opt = std::nullopt;
        std::optional<std::int64_t> user_id_opt = std::nullopt;
        std::optional<std::int64_t> problem_id_opt = std::nullopt;
        std::optional<std::string> language_opt = std::nullopt;
        std::optional<std::string> status_opt = std::nullopt;
        std::optional<std::int32_t> limit_opt = std::nullopt;
    };

    struct queued_submission{
        std::int64_t submission_id = 0;
        std::int64_t problem_id = 0;
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

    std::expected<source, dto_validation_error> make_source_from_json(
        const boost::json::object& json
    );
    std::expected<list_filter, dto_validation_error> make_list_filter_from_query_params(
        const std::vector<http_util::query_param>& query_params
    );
    std::expected<create_request, dto_validation_error> make_create_request_from_json(
        const boost::json::object& json,
        std::int64_t user_id,
        std::int64_t problem_id
    );
    history make_history_from_row(const pqxx::row& submission_history_row);
    history_list make_history_list_from_result(const pqxx::result& submission_history_result);
    source_detail make_source_detail_from_row(const pqxx::row& submission_source_row);
    detail make_detail_from_row(const pqxx::row& submission_detail_row);
    created make_created(
        std::int64_t submission_id,
        submission_status submission_status_value
    );
    summary make_summary_from_row(const pqxx::row& submission_summary_row);
    std::vector<summary> make_summary_list_from_result(const pqxx::result& submission_summary_result);
    queued_submission make_queued_submission_from_row(const pqxx::row& submission_queue_row);
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
