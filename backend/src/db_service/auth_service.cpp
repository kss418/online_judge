#include "db_service/auth_service.hpp"
#include "common/permission_util.hpp"
#include "db_service/db_service_util.hpp"
#include "db_repository/auth_repository.hpp"
#include "common/crypto_util.hpp"
#include "common/token_util.hpp"

#include <chrono>

std::expected<auth_dto::identity, service_error> auth_service::auth_token(
    db_connection& connection_value,
    const auth_dto::token& token_value
){
    const auto token_hash_exp = crypto_util::sha512_hex(token_value.value);
    if(!token_hash_exp){
        return std::unexpected(service_error(token_hash_exp.error()));
    }
    auth_dto::hashed_token hashed_token_value;
    hashed_token_value.token_hash = *token_hash_exp;

    return db_service_util::with_retry_service_read_transaction(
        connection_value,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<auth_dto::identity, service_error> {
            const auto get_token_identity_exp = auth_repository::get_token_identity(
                transaction,
                hashed_token_value
            );
            if(!get_token_identity_exp){
                return std::unexpected(get_token_identity_exp.error());
            }
            if(!get_token_identity_exp->has_value()){
                return std::unexpected(service_error::unauthorized);
            }

            return std::move(get_token_identity_exp->value());
        }
    );
}

std::expected<void, service_error> auth_service::renew_token(
    db_connection& connection_value,
    const auth_dto::token& token_value
){
    const auto token_hash_exp = crypto_util::sha512_hex(token_value.value);
    if(!token_hash_exp){
        return std::unexpected(service_error(token_hash_exp.error()));
    }
    auth_dto::hashed_token hashed_token_value;
    hashed_token_value.token_hash = *token_hash_exp;

    return db_service_util::with_retry_service_write_transaction(
        connection_value,
        [&](pqxx::work& transaction) -> std::expected<void, service_error> {
            const auto update_expires_at_exp = auth_repository::update_expires_at(
                transaction,
                hashed_token_value,
                token_util::TOKEN_TTL
            );
            if(!update_expires_at_exp){
                return std::unexpected(update_expires_at_exp.error());
            }
            if(!update_expires_at_exp.value()){
                return std::unexpected(service_error::unauthorized);
            }

            return {};
        }
    );
}

std::expected<void, service_error> auth_service::revoke_token(
    db_connection& connection_value,
    const auth_dto::token& token_value
){
    const auto token_hash_exp = crypto_util::sha512_hex(token_value.value);
    if(!token_hash_exp){
        return std::unexpected(service_error(token_hash_exp.error()));
    }
    auth_dto::hashed_token hashed_token_value;
    hashed_token_value.token_hash = *token_hash_exp;

    return db_service_util::with_retry_service_write_transaction(
        connection_value,
        [&](pqxx::work& transaction) -> std::expected<void, service_error> {
            const auto revoke_token_exp = auth_repository::revoke_token(
                transaction,
                hashed_token_value
            );
            if(!revoke_token_exp){
                return std::unexpected(revoke_token_exp.error());
            }
            if(!revoke_token_exp.value()){
                return std::unexpected(service_error::unauthorized);
            }

            return {};
        }
    );
}

std::expected<void, service_error> auth_service::update_permission_level(
    db_connection& connection_value,
    std::int64_t user_id,
    std::int32_t permission_level
){
    if(
        user_id <= 0 ||
        !permission_util::is_valid_permission_level(permission_level)
    ){
        return std::unexpected(service_error::validation_error);
    }

    return db_service_util::with_retry_service_write_transaction(
        connection_value,
        [&](pqxx::work& transaction) -> std::expected<void, service_error> {
            const auto update_permission_level_exp =
                auth_repository::update_permission_level(
                    transaction,
                    user_id,
                    permission_level
                );
            if(!update_permission_level_exp){
                return std::unexpected(update_permission_level_exp.error());
            }
            if(!*update_permission_level_exp){
                return std::unexpected(service_error::not_found);
            }

            return {};
        }
    );
}

std::expected<auth_dto::user_summary_list, service_error> auth_service::get_user_list(
    db_connection& connection_value
){
    return db_service_util::with_retry_service_read_transaction(
        connection_value,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<auth_dto::user_summary_list, service_error> {
            return auth_repository::get_user_list(transaction);
        }
    );
}
