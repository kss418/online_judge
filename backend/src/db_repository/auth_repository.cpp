#include "db_repository/auth_repository.hpp"
#include "db_repository/db_repository.hpp"

#include "common/permission_util.hpp"

std::expected<void, error_code> auth_repository::insert_token(
    pqxx::transaction_base& transaction,
    std::int64_t user_id,
    const auth_dto::hashed_token& hashed_token_value,
    std::chrono::seconds token_ttl
){
    if(
        user_id <= 0 ||
        hashed_token_value.token_hash.empty() ||
        token_ttl <= std::chrono::seconds::zero()
    ){
        return std::unexpected(
            user_id <= 0
                ? db_repository::invalid_reference_error()
                : db_repository::invalid_input_error()
        );
    }

    const auto insert_result = transaction.exec(
        "INSERT INTO auth_tokens(user_id, token_hash, expires_at) "
        "SELECT "
        "user_id, "
        "$2, "
        "NOW() + ($3 * INTERVAL '1 second') "
        "FROM users "
        "WHERE user_id = $1 "
        "RETURNING token_id",
        pqxx::params{user_id, hashed_token_value.token_hash, token_ttl.count()}
    );

    if(insert_result.empty()){
        return std::unexpected(db_repository::not_found_error());
    }

    return {};
}

std::expected<bool, error_code> auth_repository::revoke_token(
    pqxx::transaction_base& transaction,
    const auth_dto::hashed_token& hashed_token_value
){
    if(hashed_token_value.token_hash.empty()){
        return std::unexpected(db_repository::invalid_input_error());
    }

    const auto revoke_result = transaction.exec(
        "UPDATE auth_tokens "
        "SET revoked_at = NOW() "
        "WHERE token_hash = $1 AND revoked_at IS NULL AND expires_at > NOW()",
        pqxx::params{hashed_token_value.token_hash}
    );

    return revoke_result.affected_rows() > 0;
}

std::expected<std::optional<auth_dto::identity>, error_code> auth_repository::get_token_identity(
    pqxx::transaction_base& transaction,
    const auth_dto::hashed_token& hashed_token_value
){
    if(hashed_token_value.token_hash.empty()){
        return std::unexpected(db_repository::invalid_input_error());
    }

    const auto token_identity_result = transaction.exec(
        "SELECT token_table.user_id, user_table.permission_level, "
        "COALESCE(user_table.user_login_id, '') "
        "FROM auth_tokens token_table "
        "JOIN users user_table "
        "ON token_table.user_id = user_table.user_id "
        "WHERE "
        "token_table.token_hash = $1 AND "
        "token_table.revoked_at IS NULL AND "
        "token_table.expires_at > NOW() ",
        pqxx::params{hashed_token_value.token_hash}
    );

    if(token_identity_result.empty()){
        return std::nullopt;
    }

    auth_dto::identity identity_value;
    identity_value.user_id = token_identity_result[0][0].as<std::int64_t>();
    identity_value.permission_level = token_identity_result[0][1].as<std::int32_t>();
    identity_value.user_login_id = token_identity_result[0][2].as<std::string>();
    return identity_value;
}

std::expected<bool, error_code> auth_repository::update_permission_level(
    pqxx::transaction_base& transaction,
    std::int64_t user_id,
    std::int32_t permission_level
){
    if(
        user_id <= 0 ||
        !permission_util::is_valid_permission_level(permission_level)
    ){
        return std::unexpected(
            user_id <= 0
                ? db_repository::invalid_reference_error()
                : db_repository::invalid_input_error()
        );
    }

    const auto update_result = transaction.exec(
        "WITH updated_user AS ("
        "    UPDATE users "
        "    SET "
        "    permission_level = $2, "
        "    auth_updated_at = NOW() "
        "    WHERE user_id = $1 "
        "    RETURNING user_id"
        ") "
        "UPDATE user_info "
        "SET updated_at = NOW() "
        "WHERE user_id IN (SELECT user_id FROM updated_user)",
        pqxx::params{user_id, permission_level}
    );

    return update_result.affected_rows() > 0;
}

std::expected<auth_dto::user_summary_list, error_code> auth_repository::get_user_list(
    pqxx::transaction_base& transaction
){
    const auto user_list_result = transaction.exec(
        "SELECT "
        "user_table.user_id, "
        "COALESCE(user_table.user_login_id, ''), "
        "user_table.permission_level, "
        "user_info_table.created_at::text "
        "FROM users user_table "
        "JOIN user_info user_info_table "
        "ON user_info_table.user_id = user_table.user_id "
        "ORDER BY user_table.permission_level DESC, user_table.user_id ASC"
    );

    auth_dto::user_summary_list user_summary_values;
    user_summary_values.reserve(user_list_result.size());

    for(const auto& user_row : user_list_result){
        auth_dto::user_summary user_summary_value;
        user_summary_value.user_id = user_row[0].as<std::int64_t>();
        user_summary_value.user_login_id = user_row[1].as<std::string>();
        user_summary_value.permission_level = user_row[2].as<std::int32_t>();
        user_summary_value.created_at = user_row[3].as<std::string>();
        user_summary_values.push_back(std::move(user_summary_value));
    }

    return user_summary_values;
}

std::expected<bool, error_code> auth_repository::update_expires_at(
    pqxx::transaction_base& transaction,
    const auth_dto::hashed_token& hashed_token_value,
    std::chrono::seconds token_ttl
){
    if(
        hashed_token_value.token_hash.empty() ||
        token_ttl <= std::chrono::seconds::zero()
    ){
        return std::unexpected(db_repository::invalid_input_error());
    }

    const auto update_result = transaction.exec(
        "UPDATE auth_tokens "
        "SET expires_at = NOW() + ($2 * INTERVAL '1 second') "
        "WHERE "
        "token_hash = $1 AND "
        "revoked_at IS NULL AND "
        "expires_at > NOW()",
        pqxx::params{hashed_token_value.token_hash, token_ttl.count()}
    );

    return update_result.affected_rows() > 0;
}
