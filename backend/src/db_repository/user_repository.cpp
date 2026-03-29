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

std::expected<std::optional<std::string>, error_code> user_repository::create_submission_ban(
    pqxx::transaction_base& transaction,
    std::int64_t user_id,
    std::int32_t duration_minutes
){
    if(user_id <= 0 || duration_minutes <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto update_result = transaction.exec(
        "UPDATE user_info "
        "SET "
        "submission_banned_until = NOW() + ($2 * INTERVAL '1 minute'), "
        "updated_at = NOW() "
        "WHERE user_id = $1 "
        "RETURNING submission_banned_until::text",
        pqxx::params{user_id, duration_minutes}
    );

    if(update_result.empty()){
        return std::nullopt;
    }

    return update_result[0][0].as<std::string>();
}

std::expected<std::optional<user_dto::submission_ban_status>, error_code>
user_repository::get_submission_ban_status(
    pqxx::transaction_base& transaction,
    std::int64_t user_id
){
    if(user_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto user_info_result = transaction.exec(
        "SELECT submission_banned_until::text "
        "FROM user_info "
        "WHERE user_id = $1",
        pqxx::params{user_id}
    );

    if(user_info_result.empty()){
        return std::optional<user_dto::submission_ban_status>{std::nullopt};
    }

    user_dto::submission_ban_status submission_ban_status_value;
    submission_ban_status_value.user_id = user_id;
    if(!user_info_result[0][0].is_null()){
        submission_ban_status_value.submission_banned_until_opt =
            user_info_result[0][0].as<std::string>();
    }

    return submission_ban_status_value;
}

std::expected<std::optional<std::string>, error_code>
user_repository::get_active_submission_banned_until(
    pqxx::transaction_base& transaction,
    std::int64_t user_id
){
    if(user_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto user_info_result = transaction.exec(
        "SELECT "
        "CASE "
        "WHEN submission_banned_until IS NOT NULL AND submission_banned_until > NOW() "
        "THEN submission_banned_until::text "
        "ELSE NULL "
        "END "
        "FROM user_info "
        "WHERE user_id = $1 "
        "FOR UPDATE",
        pqxx::params{user_id}
    );

    if(user_info_result.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }
    if(user_info_result[0][0].is_null()){
        return std::optional<std::string>{std::nullopt};
    }

    return user_info_result[0][0].as<std::string>();
}

std::expected<bool, error_code> user_repository::update_submission_banned_until(
    pqxx::transaction_base& transaction,
    std::int64_t user_id,
    std::string_view submission_banned_until
){
    if(user_id <= 0 || submission_banned_until.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto update_result = transaction.exec(
        "UPDATE user_info "
        "SET "
        "submission_banned_until = $2::timestamptz, "
        "updated_at = NOW() "
        "WHERE user_id = $1",
        pqxx::params{user_id, submission_banned_until}
    );

    return update_result.affected_rows() > 0;
}

std::expected<bool, error_code> user_repository::clear_submission_banned_until(
    pqxx::transaction_base& transaction,
    std::int64_t user_id
){
    if(user_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto update_result = transaction.exec(
        "UPDATE user_info "
        "SET "
        "submission_banned_until = NULL, "
        "updated_at = NOW() "
        "WHERE user_id = $1",
        pqxx::params{user_id}
    );

    return update_result.affected_rows() > 0;
}
