#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace user_dto{
    struct summary{
        std::int64_t user_id = 0;
        std::string user_login_id;
        std::string created_at;
    };

    struct list_filter{
        std::optional<std::string> query_opt = std::nullopt;
    };

    struct list_item{
        std::int64_t user_id = 0;
        std::string user_login_id;
        std::int64_t solved_problem_count = 0;
        std::int64_t accepted_submission_count = 0;
        std::int64_t submission_count = 0;
        std::string created_at;
    };

    struct submission_ban_request{
        std::int32_t duration_minutes = 0;
    };

    struct submission_ban{
        std::int64_t user_id = 0;
        std::int32_t duration_minutes = 0;
        std::string submission_banned_until;
    };

    struct submission_ban_status{
        std::int64_t user_id = 0;
        std::optional<std::string> submission_banned_until_opt = std::nullopt;
    };

    using list = std::vector<list_item>;
}
