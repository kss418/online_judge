#pragma once

#include "common/submission_status.hpp"

#include <boost/json/fwd.hpp>

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>

namespace submission_dto{
    struct source{
        std::string language;
        std::string source_code;
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
        std::int64_t problem_id = 0;
        std::string language;
        std::string status;
        std::optional<std::int16_t> score = std::nullopt;
        std::string created_at;
        std::string updated_at;
    };

    struct list_filter{
        std::optional<std::int64_t> top_submission_id_opt = std::nullopt;
        std::optional<std::int64_t> user_id_opt = std::nullopt;
        std::optional<std::int64_t> problem_id_opt = std::nullopt;
        std::optional<std::string> status_opt = std::nullopt;
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
        std::optional<std::string> reason_opt = std::nullopt;
    };

    struct finalize_result{
        std::int64_t problem_id = 0;
        bool should_increase_accepted_count = false;
    };

    std::optional<source> make_source(const boost::json::object& request_object);
}
