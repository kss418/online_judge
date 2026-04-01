#include "query_builder/user_query_builder.hpp"

#include "common/permission_util.hpp"

user_query_builder::public_user_list_query_builder::public_user_list_query_builder(
    const user_dto::list_filter& filter_value
) :
    filter_value_(filter_value){}

assembled_query user_query_builder::public_user_list_query_builder::build() const{
    std::string query =
        "SELECT "
        "user_table.user_id, "
        "COALESCE(user_table.user_login_id, ''), "
        "COALESCE(solved_problem_counts.solved_problem_count, 0)::BIGINT, "
        "COALESCE(accepted_submission_counts.accepted_submission_count, 0)::BIGINT, "
        "COALESCE(submission_counts.submission_count, 0)::BIGINT, "
        "user_info_table.created_at::text "
        "FROM users user_table "
        "JOIN user_info user_info_table "
        "ON user_info_table.user_id = user_table.user_id "
        "LEFT JOIN ("
        "    SELECT "
        "    user_id, "
        "    COUNT(*)::BIGINT AS submission_count "
        "    FROM submissions "
        "    GROUP BY user_id"
        ") submission_counts "
        "ON submission_counts.user_id = user_table.user_id "
        "LEFT JOIN ("
        "    SELECT "
        "    user_id, "
        "    COUNT(*)::BIGINT AS accepted_submission_count "
        "    FROM submissions "
        "    WHERE status = 'accepted'::submission_status "
        "    GROUP BY user_id"
        ") accepted_submission_counts "
        "ON accepted_submission_counts.user_id = user_table.user_id "
        "LEFT JOIN ("
        "    SELECT "
        "    user_id, "
        "    COUNT(*)::BIGINT AS solved_problem_count "
        "    FROM ("
        "        SELECT "
        "        user_id, "
        "        problem_id "
        "        FROM submissions "
        "        WHERE status = 'accepted'::submission_status "
        "        GROUP BY user_id, problem_id"
        "    ) solved_problem_values "
        "    GROUP BY user_id"
        ") solved_problem_counts "
        "ON solved_problem_counts.user_id = user_table.user_id ";

    pqxx::params params;
    params.append(permission_util::SUPERADMIN);

    if(filter_value_.query_opt){
        query +=
            "WHERE "
            "user_table.permission_level < $1 AND "
            "user_table.user_login_id ILIKE ('%' || $2 || '%') ";
        params.append(*filter_value_.query_opt);
    }
    else{
        query +=
            "WHERE "
            "user_table.permission_level < $1 ";
    }

    query += "ORDER BY user_table.user_id ASC";

    return assembled_query{
        .sql = std::move(query),
        .params = std::move(params)
    };
}
