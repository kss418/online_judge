#include "db/auth_util.hpp"

std::expected<void, error_code> auth_util::insert_token(
    pqxx::transaction_base& transaction,
    std::int64_t user_id,
    std::string_view token_hash,
    std::chrono::seconds token_ttl
){
    if(user_id <= 0 || token_hash.empty() || token_ttl <= std::chrono::seconds::zero()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
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
        pqxx::params{user_id, token_hash, token_ttl.count()}
    );

    if(insert_result.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return {};
}

std::expected<bool, error_code> auth_util::revoke_token(
    pqxx::transaction_base& transaction,
    std::string_view token_hash
){
    if(token_hash.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto revoke_result = transaction.exec(
        "UPDATE auth_tokens "
        "SET revoked_at = NOW() "
        "WHERE token_hash = $1 AND revoked_at IS NULL",
        pqxx::params{token_hash}
    );

    return revoke_result.affected_rows() > 0;
}

std::expected<std::optional<auth_util::token_identity>, error_code> auth_util::get_token_identity(
    pqxx::transaction_base& transaction,
    std::string_view token_hash
){
    if(token_hash.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto token_identity_result = transaction.exec(
        "SELECT token_table.user_id, user_table.is_admin "
        "FROM auth_tokens token_table "
        "JOIN users user_table "
        "ON token_table.user_id = user_table.user_id "
        "WHERE "
        "token_table.token_hash = $1 AND "
        "token_table.revoked_at IS NULL AND "
        "token_table.expires_at > NOW() "
        "FOR UPDATE OF token_table",
        pqxx::params{token_hash}
    );

    if(token_identity_result.empty()){
        return std::nullopt;
    }

    token_identity token_identity_value;
    token_identity_value.user_id = token_identity_result[0][0].as<std::int64_t>();
    token_identity_value.is_admin = token_identity_result[0][1].as<bool>();
    return token_identity_value;
}

std::expected<void, error_code> auth_util::update_last_used_at(
    pqxx::transaction_base& transaction,
    std::string_view token_hash
){
    if(token_hash.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto update_result = transaction.exec(
        "UPDATE auth_tokens "
        "SET last_used_at = NOW() "
        "WHERE "
        "token_hash = $1 AND "
        "revoked_at IS NULL AND "
        "expires_at > NOW()",
        pqxx::params{token_hash}
    );

    if(update_result.affected_rows() == 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return {};
}

std::expected<bool, error_code> auth_util::update_admin_status(
    pqxx::transaction_base& transaction,
    std::int64_t user_id,
    bool is_admin
){
    if(user_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto update_result = transaction.exec(
        "UPDATE users "
        "SET is_admin = $2, updated_at = NOW() "
        "WHERE user_id = $1",
        pqxx::params{user_id, is_admin}
    );

    return update_result.affected_rows() > 0;
}

std::expected<bool, error_code> auth_util::update_expires_at(
    pqxx::transaction_base& transaction,
    std::string_view token_hash,
    std::chrono::seconds token_ttl
){
    if(token_hash.empty() || token_ttl <= std::chrono::seconds::zero()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto update_result = transaction.exec(
        "UPDATE auth_tokens "
        "SET expires_at = NOW() + ($2 * INTERVAL '1 second') "
        "WHERE "
        "token_hash = $1 AND "
        "revoked_at IS NULL AND "
        "expires_at > NOW()",
        pqxx::params{token_hash, token_ttl.count()}
    );

    return update_result.affected_rows() > 0;
}
