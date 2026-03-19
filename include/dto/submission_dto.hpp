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

    std::optional<source> make_source(const boost::json::object& request_object);
}
