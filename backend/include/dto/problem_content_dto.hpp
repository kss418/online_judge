#pragma once

#include "dto/dto_validation_error.hpp"

#include <boost/json/fwd.hpp>

#include <cstdint>
#include <expected>
#include <optional>
#include <string>

namespace problem_content_dto{
    struct limits{
        std::int32_t memory_mb = 0;
        std::int32_t time_ms = 0;
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

    bool is_valid(const limits& limits_value);
    bool is_valid(const statement& statement_value);
    bool is_valid(const sample_ref& sample_reference_value);

    std::expected<limits, dto_validation_error> make_limits_from_json(
        const boost::json::object& json
    );
    std::expected<statement, dto_validation_error> make_statement_from_json(
        const boost::json::object& json
    );
    std::expected<sample, dto_validation_error> make_sample_from_json(
        const boost::json::object& json
    );
}
