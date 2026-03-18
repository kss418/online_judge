#pragma once

#include <boost/json/fwd.hpp>

#include <cstdint>
#include <optional>
#include <string>

namespace problem_dto{
    struct limits{
        std::int32_t memory_mb = 0;
        std::int32_t time_ms = 0;
    };

    struct tc{
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

    struct sample{
        std::int64_t id = 0;
        std::int32_t order = 0;
        std::string input;
        std::string output;
    };

    struct statistics{
        std::int64_t submission_count = 0;
        std::int64_t accepted_count = 0;
    };

    std::optional<limits> make_limits(const boost::json::object& request_object);
    std::optional<statement> make_statement(const boost::json::object& request_object);
    std::optional<tc> make_tc(const boost::json::object& request_object);
}
