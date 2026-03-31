#pragma once

#include "dto/dto_validation_error.hpp"

#include <boost/json/fwd.hpp>

#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace request_parser{
    struct query_param;
}

namespace pqxx{
    class result;
    class row;
}

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

    std::expected<submission_ban_request, dto_validation_error>
    make_submission_ban_request_from_json(const boost::json::object& json);
    std::expected<list_filter, dto_validation_error> make_list_filter_from_query_params(
        const std::vector<request_parser::query_param>& query_params
    );
    summary make_summary_from_row(const pqxx::row& user_summary_row);
    list_item make_list_item_from_row(const pqxx::row& user_list_row);
    list make_list_from_result(const pqxx::result& user_list_result);
}
