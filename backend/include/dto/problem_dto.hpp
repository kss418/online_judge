#pragma once

#include "dto/problem_content_dto.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace problem_dto{
    struct reference{
        std::int64_t problem_id = 0;
    };

    struct title{
        std::string value;
    };

    struct existence{
        bool exists = false;
    };

    struct create_request{
        std::string title;
    };

    struct update_request{
        std::string title;
    };

    struct created{
        std::int64_t problem_id = 0;
    };

    struct summary{
        std::int64_t problem_id = 0;
        std::string title;
        std::int32_t version = 0;
        std::int32_t time_limit_ms = 0;
        std::int32_t memory_limit_mb = 0;
        std::int64_t submission_count = 0;
        std::int64_t accepted_count = 0;
        std::optional<std::string> user_problem_state_opt = std::nullopt;
    };

    struct version{
        std::int32_t version = 0;
    };

    struct detail{
        reference problem_reference_value;
        title title_value;
        version version_value;
        problem_content_dto::limits limits_value;
        std::optional<problem_content_dto::statement> statement_opt = std::nullopt;
        std::vector<problem_content_dto::sample> sample_values;
        problem_content_dto::statistics statistics_value;
        std::optional<std::string> user_problem_state_opt = std::nullopt;
    };

    struct testcase_ref{
        std::int64_t problem_id = 0;
        std::int32_t testcase_order = 0;
    };

    struct testcase_count{
        std::int32_t testcase_count = 0;
    };

    struct testcase_move_request{
        std::int32_t source_testcase_order = 0;
        std::int32_t target_testcase_order = 0;
    };

    struct testcase{
        std::int64_t id = 0;
        std::int32_t order = 0;
        std::string input;
        std::string output;
    };

    struct testcase_summary{
        std::int64_t id = 0;
        std::int32_t order = 0;
        std::int32_t input_char_count = 0;
        std::int32_t input_line_count = 0;
        std::int32_t output_char_count = 0;
        std::int32_t output_line_count = 0;
    };

    struct list_filter{
        std::optional<std::string> title_opt = std::nullopt;
        std::optional<std::int64_t> problem_id_opt = std::nullopt;
        std::optional<std::string> state_opt = std::nullopt;
        std::optional<std::string> sort_opt = std::nullopt;
        std::optional<std::string> direction_opt = std::nullopt;
        std::optional<std::int32_t> limit_opt = std::nullopt;
        std::optional<std::int64_t> offset_opt = std::nullopt;
    };

    bool is_valid(const reference& reference_value);
    bool is_valid(const title& title_value);
    bool is_valid(const create_request& create_request_value);
    bool is_valid(const update_request& update_request_value);
    bool is_valid(const testcase_ref& testcase_reference_value);
}
