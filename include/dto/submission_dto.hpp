#pragma once

#include <boost/json/fwd.hpp>

#include <cstdint>
#include <optional>
#include <string>

namespace submission_dto{
    struct source{
        std::string language;
        std::string source_code;
    };

    struct created{
        std::int64_t submission_id = 0;
        std::string status;
    };

    struct detail{
        std::int64_t submission_id = 0;
        std::int64_t user_id = 0;
        std::int64_t problem_id = 0;
        std::string language;
        std::string source_code;
        std::string status;
        std::optional<std::int16_t> score = std::nullopt;
        std::optional<std::string> compile_output = std::nullopt;
        std::optional<std::string> judge_output = std::nullopt;
        std::string created_at;
        std::string updated_at;
    };

    struct queued_submission{
        std::int64_t submission_id = 0;
        std::int64_t problem_id = 0;
        std::string language;
        std::string source_code;
    };

    std::optional<source> make_source(const boost::json::object& request_object);
}
