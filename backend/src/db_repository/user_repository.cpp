#include "db_repository/user_repository.hpp"
#include "error/repository_error.hpp"
#include "query_builder/user_query_builder.hpp"
#include "row_mapper/user_row_mapper.hpp"

#include <pqxx/pqxx>

std::expected<bool, repository_error> user_repository::exists_user(
    pqxx::transaction_base& transaction,
    std::int64_t user_id
){
    if(user_id <= 0){
        return std::unexpected(repository_error::invalid_reference);
    }

    const auto exists_result = transaction.exec(
        "SELECT EXISTS("
        "SELECT 1 "
        "FROM users "
        "WHERE user_id = $1"
        ")",
        pqxx::params{user_id}
    );

    if(exists_result.empty()){
        return std::unexpected(repository_error::internal);
    }

    return exists_result[0][0].as<bool>();
}

std::expected<user_dto::list, repository_error> user_repository::get_public_list(
    pqxx::transaction_base& transaction,
    const user_dto::list_filter& filter_value
){
    const auto query_value = user_query_builder::public_user_list_query_builder{
        filter_value
    }.build();
    const auto user_list_result = transaction.exec(
        query_value.sql,
        std::move(query_value.params)
    );
    return user_row_mapper::map_list_result(user_list_result);
}

std::expected<user_dto::summary, repository_error> user_repository::get_summary(
    pqxx::transaction_base& transaction,
    std::int64_t user_id
){
    if(user_id <= 0){
        return std::unexpected(repository_error::invalid_reference);
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
        return std::unexpected(repository_error::not_found);
    }

    return user_row_mapper::map_summary_row(user_summary_result[0]);
}

std::expected<user_dto::summary, repository_error>
user_repository::get_summary_by_login_id(
    pqxx::transaction_base& transaction,
    std::string_view user_login_id
){
    if(user_login_id.empty()){
        return std::unexpected(repository_error::invalid_input);
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
        return std::unexpected(repository_error::not_found);
    }

    return user_row_mapper::map_summary_row(user_summary_result[0]);
}

std::expected<std::string, repository_error> user_repository::create_submission_ban(
    pqxx::transaction_base& transaction,
    std::int64_t user_id,
    std::int32_t duration_minutes
){
    if(user_id <= 0 || duration_minutes <= 0){
        return std::unexpected(
            user_id <= 0
                ? repository_error::invalid_reference
                : repository_error::invalid_input
        );
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
        return std::unexpected(repository_error::not_found);
    }

    return update_result[0][0].as<std::string>();
}

std::expected<user_dto::submission_ban_status, repository_error>
user_repository::get_submission_ban_status(
    pqxx::transaction_base& transaction,
    std::int64_t user_id
){
    if(user_id <= 0){
        return std::unexpected(repository_error::invalid_reference);
    }

    const auto user_info_result = transaction.exec(
        "SELECT submission_banned_until::text "
        "FROM user_info "
        "WHERE user_id = $1",
        pqxx::params{user_id}
    );

    if(user_info_result.empty()){
        return std::unexpected(repository_error::not_found);
    }

    user_dto::submission_ban_status submission_ban_status_value;
    submission_ban_status_value.user_id = user_id;
    if(!user_info_result[0][0].is_null()){
        submission_ban_status_value.submission_banned_until_opt =
            user_info_result[0][0].as<std::string>();
    }

    return submission_ban_status_value;
}

std::expected<std::optional<std::string>, repository_error>
user_repository::get_active_submission_banned_until(
    pqxx::transaction_base& transaction,
    std::int64_t user_id
){
    if(user_id <= 0){
        return std::unexpected(repository_error::invalid_reference);
    }

    const auto user_info_result = transaction.exec(
        "SELECT "
        "CASE "
        "WHEN submission_banned_until IS NOT NULL AND submission_banned_until > NOW() "
        "THEN submission_banned_until::text "
        "ELSE NULL "
        "END "
        "FROM user_info "
        "WHERE user_id = $1",
        pqxx::params{user_id}
    );

    if(user_info_result.empty()){
        return std::unexpected(repository_error::not_found);
    }
    if(user_info_result[0][0].is_null()){
        return std::optional<std::string>{std::nullopt};
    }

    return user_info_result[0][0].as<std::string>();
}

std::expected<void, repository_error> user_repository::clear_submission_banned_until(
    pqxx::transaction_base& transaction,
    std::int64_t user_id
){
    if(user_id <= 0){
        return std::unexpected(repository_error::invalid_reference);
    }

    const auto update_result = transaction.exec(
        "UPDATE user_info "
        "SET "
        "submission_banned_until = NULL, "
        "updated_at = NOW() "
        "WHERE user_id = $1",
        pqxx::params{user_id}
    );

    if(update_result.affected_rows() == 0){
        return std::unexpected(repository_error::not_found);
    }

    return {};
}
