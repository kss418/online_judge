#pragma once

#include "dto/dto_validation_error.hpp"

#include <boost/json/fwd.hpp>

#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <vector>

namespace http_util{
    struct query_param;
}

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

    struct created{
        std::int64_t problem_id = 0;
    };

    struct summary{
        std::int64_t problem_id = 0;
        std::string title;
        std::int32_t version = 0;
    };

    struct version{
        std::int32_t version = 0;
    };

    struct limits{
        std::int32_t memory_mb = 0;
        std::int32_t time_ms = 0;
    };

    struct testcase_ref{
        std::int64_t problem_id = 0;
        std::int32_t testcase_order = 0;
    };

    struct testcase_count{
        std::int32_t testcase_count = 0;
    };

    struct testcase{
        std::int64_t id = 0;
        std::int32_t order = 0;
        std::string input;
        std::string output;
    };

    struct statement{
        std::string description;
        std::string input_format;
        std::string output_format;
        std::optional<std::string> note = std::nullopt;
    };

    struct sample_ref{
        std::int64_t problem_id = 0;
        std::int32_t sample_order = 0;
    };

    struct sample{
        std::int64_t id = 0;
        std::int32_t order = 0;
        std::string input;
        std::string output;
    };

    struct sample_count{
        std::int32_t sample_count = 0;
    };

    struct statistics{
        std::int64_t submission_count = 0;
        std::int64_t accepted_count = 0;
    };

    struct list_filter{
        std::optional<std::string> title_opt = std::nullopt;
    };

    std::expected<create_request, dto_validation_error> make_create_request_from_json(
        const boost::json::object& json
    );
    std::expected<list_filter, dto_validation_error> make_list_filter_from_query_params(
        const std::vector<http_util::query_param>& query_params
    );
    std::expected<limits, dto_validation_error> make_limits_from_json(
        const boost::json::object& json
    );
    std::expected<statement, dto_validation_error> make_statement_from_json(
        const boost::json::object& json
    );
    std::expected<sample, dto_validation_error> make_sample_from_json(
        const boost::json::object& json
    );
    std::expected<testcase, dto_validation_error> make_testcase_from_json(
        const boost::json::object& json
    );
}
