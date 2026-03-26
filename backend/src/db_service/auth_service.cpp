#include "db_service/auth_service.hpp"
#include "common/permission_util.hpp"
#include "db_service/db_service_util.hpp"
#include "db_util/auth_util.hpp"
#include "common/crypto_util.hpp"
#include "common/token_util.hpp"

#include <chrono>

std::expected<std::optional<auth_dto::identity>, error_code> auth_service::auth_token(
    db_connection& connection_value,
    const auth_dto::token& token_value
){
    if(token_value.value.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto token_hash_exp = crypto_util::sha512_hex(token_value.value);
    if(!token_hash_exp){
        return std::unexpected(token_hash_exp.error());
    }
    auth_dto::hashed_token hashed_token_value;
    hashed_token_value.token_hash = *token_hash_exp;

    return db_service_util::with_retry_write_transaction(
        connection_value,
        [&](pqxx::work& transaction)
            -> std::expected<std::optional<auth_dto::identity>, error_code> {
            const auto get_token_identity_exp = auth_util::get_token_identity(
                transaction,
                hashed_token_value
            );
            if(!get_token_identity_exp){
                return std::unexpected(get_token_identity_exp.error());
            }
            if(!get_token_identity_exp->has_value()){
                return std::nullopt;
            }

            const auto update_last_used_at_exp = auth_util::update_last_used_at(
                transaction,
                hashed_token_value
            );
            if(!update_last_used_at_exp){
                return std::unexpected(update_last_used_at_exp.error());
            }

            return get_token_identity_exp->value();
        }
    );
}

std::expected<bool, error_code> auth_service::renew_token(
    db_connection& connection_value,
    const auth_dto::token& token_value
){
    if(token_value.value.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto token_hash_exp = crypto_util::sha512_hex(token_value.value);
    if(!token_hash_exp){
        return std::unexpected(token_hash_exp.error());
    }
    auth_dto::hashed_token hashed_token_value;
    hashed_token_value.token_hash = *token_hash_exp;

    return db_service_util::with_retry_write_transaction(
        connection_value,
        [&](pqxx::work& transaction) -> std::expected<bool, error_code> {
            const auto update_expires_at_exp = auth_util::update_expires_at(
                transaction,
                hashed_token_value,
                token_util::TOKEN_TTL
            );
            if(!update_expires_at_exp){
                return std::unexpected(update_expires_at_exp.error());
            }
            if(!update_expires_at_exp.value()){
                return false;
            }

            return true;
        }
    );
}

std::expected<bool, error_code> auth_service::revoke_token(
    db_connection& connection_value,
    const auth_dto::token& token_value
){
    if(token_value.value.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto token_hash_exp = crypto_util::sha512_hex(token_value.value);
    if(!token_hash_exp){
        return std::unexpected(token_hash_exp.error());
    }
    auth_dto::hashed_token hashed_token_value;
    hashed_token_value.token_hash = *token_hash_exp;

    return db_service_util::with_retry_write_transaction(
        connection_value,
        [&](pqxx::work& transaction) -> std::expected<bool, error_code> {
            const auto revoke_token_exp = auth_util::revoke_token(
                transaction,
                hashed_token_value
            );
            if(!revoke_token_exp){
                return std::unexpected(revoke_token_exp.error());
            }
            if(!revoke_token_exp.value()){
                return false;
            }

            return true;
        }
    );
}

std::expected<bool, error_code> auth_service::update_permission_level(
    db_connection& connection_value,
    std::int64_t user_id,
    std::int32_t permission_level
){
    if(
        user_id <= 0 ||
        !permission_util::is_valid_permission_level(permission_level)
    ){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return db_service_util::with_retry_write_transaction(
        connection_value,
        [&](pqxx::work& transaction) -> std::expected<bool, error_code> {
            const auto update_permission_level_exp = auth_util::update_permission_level(
                transaction,
                user_id,
                permission_level
            );
            if(!update_permission_level_exp){
                return std::unexpected(update_permission_level_exp.error());
            }

            return *update_permission_level_exp;
        }
    );
}

std::expected<auth_dto::user_summary_list, error_code> auth_service::get_user_list(
    db_connection& connection_value
){
    return db_service_util::with_retry_read_transaction(
        connection_value,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<auth_dto::user_summary_list, error_code> {
            return auth_util::get_user_list(transaction);
        }
    );
}
