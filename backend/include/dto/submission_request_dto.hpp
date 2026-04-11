#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace submission_request_dto{
    inline constexpr std::int32_t DEFAULT_LIST_LIMIT = 50;

    struct submit_request{
        std::string language;
        std::string source_code;
    };

    struct status_batch_request{
        std::vector<std::int64_t> submission_ids;
    };

    struct list_filter{
        std::optional<std::int64_t> user_id_opt = std::nullopt;
        std::optional<std::string> user_login_id_opt = std::nullopt;
        std::optional<std::int64_t> problem_id_opt = std::nullopt;
        std::optional<std::string> language_opt = std::nullopt;
        std::optional<std::string> status_opt = std::nullopt;
        std::optional<std::int32_t> limit_opt = std::nullopt;
        std::optional<std::int64_t> before_submission_id_opt = std::nullopt;
    };

    bool is_valid(const submit_request& submit_request_value);
}
