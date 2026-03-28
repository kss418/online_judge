#pragma once

#include "dto/dto_validation_error.hpp"

#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace http_util{
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

    using list = std::vector<list_item>;

    std::expected<list_filter, dto_validation_error> make_list_filter_from_query_params(
        const std::vector<http_util::query_param>& query_params
    );
    summary make_summary_from_row(const pqxx::row& user_summary_row);
    list_item make_list_item_from_row(const pqxx::row& user_list_row);
    list make_list_from_result(const pqxx::result& user_list_result);
}
