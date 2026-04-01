#pragma once

#include "db_repository/sql_filter_builder.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace viewer_problem_state_sql{
    std::string make_state_select_expr(
        std::optional<std::int64_t> viewer_user_id_opt,
        std::string_view summary_alias
    );

    void append_viewer_join(
        std::string& query,
        sql_filter_builder& predicates,
        std::optional<std::int64_t> viewer_user_id_opt,
        std::string_view summary_alias,
        std::string_view problem_id_expr
    );
}
