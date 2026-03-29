#include "db_repository/user_repository.hpp"

#include "common/permission_util.hpp"

#include <pqxx/pqxx>

std::expected<user_dto::list, error_code> user_repository::get_public_list(
    pqxx::transaction_base& transaction,
    const user_dto::list_filter& filter_value
){
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

    if(filter_value.query_opt){
        query +=
            "WHERE "
            "user_table.permission_level < $1 AND "
            "user_table.user_login_id ILIKE ('%' || $2 || '%') ";

        query += "ORDER BY user_table.user_id ASC";
        const auto user_list_result = transaction.exec(
            query,
            pqxx::params{
                permission_util::SUPERADMIN,
                *filter_value.query_opt
            }
        );
        return user_dto::make_list_from_result(user_list_result);
    }

    query +=
        "WHERE "
        "user_table.permission_level < $1 "
        "ORDER BY user_table.user_id ASC";
    const auto user_list_result = transaction.exec(
        query,
        pqxx::params{permission_util::SUPERADMIN}
    );
    return user_dto::make_list_from_result(user_list_result);
}

std::expected<std::optional<user_dto::summary>, error_code> user_repository::get_summary(
    pqxx::transaction_base& transaction,
    std::int64_t user_id
){
    if(user_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto user_summary_result = transaction.exec(
        "SELECT "
        "user_table.user_id, "
        "COALESCE(user_table.user_login_id, ''), "
        "user_info_table.created_at::text "
        "FROM users user_table "
        "JOIN user_info user_info_table "
        "ON user_info_table.user_id = user_table.user_id "
        "WHERE user_table.user_id = $1",
        pqxx::params{user_id}
    );

    if(user_summary_result.empty()){
        return std::nullopt;
    }

    return user_dto::make_summary_from_row(user_summary_result[0]);
}

std::expected<std::optional<user_dto::summary>, error_code>
user_repository::get_summary_by_login_id(
    pqxx::transaction_base& transaction,
    std::string_view user_login_id
){
    if(user_login_id.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto user_summary_result = transaction.exec(
        "SELECT "
        "user_table.user_id, "
        "COALESCE(user_table.user_login_id, ''), "
        "user_info_table.created_at::text "
        "FROM users user_table "
        "JOIN user_info user_info_table "
        "ON user_info_table.user_id = user_table.user_id "
        "WHERE user_table.user_login_id = $1",
        pqxx::params{user_login_id}
    );

    if(user_summary_result.empty()){
        return std::nullopt;
    }

    return user_dto::make_summary_from_row(user_summary_result[0]);
}
