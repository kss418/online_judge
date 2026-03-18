#include "db/auth_service.hpp"
#include "db/auth_util.hpp"
#include "common/crypto_util.hpp"
#include "common/token_util.hpp"

#include <pqxx/pqxx>

#include <chrono>

std::expected<std::optional<auth_dto::identity>, error_code> auth_service::auth_token(
    db_connection& connection_value,
    std::string_view token
){
    if(!connection_value.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(token.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto token_hash_exp = crypto_util::sha512_hex(token);
    if(!token_hash_exp){
        return std::unexpected(token_hash_exp.error());
    }

    try{
        pqxx::work transaction(connection_value.connection());
        const auto get_token_identity_exp = auth_util::get_token_identity(
            transaction,
            *token_hash_exp
        );
        if(!get_token_identity_exp){
            return std::unexpected(get_token_identity_exp.error());
        }
        if(!get_token_identity_exp->has_value()){
            return std::nullopt;
        }

        const auto update_last_used_at_exp = auth_util::update_last_used_at(
            transaction,
            *token_hash_exp
        );
        if(!update_last_used_at_exp){
            return std::unexpected(update_last_used_at_exp.error());
        }

        const auth_util::token_identity& token_identity_value = get_token_identity_exp->value();
        auth_dto::identity identity_value;
        identity_value.user_id = token_identity_value.user_id;
        identity_value.is_admin = token_identity_value.is_admin;

        transaction.commit();
        return identity_value;
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<bool, error_code> auth_service::renew_token(
    db_connection& connection_value,
    std::string_view token
){
    if(!connection_value.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(token.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto token_hash_exp = crypto_util::sha512_hex(token);
    if(!token_hash_exp){
        return std::unexpected(token_hash_exp.error());
    }

    try{
        pqxx::work transaction(connection_value.connection());
        const auto update_expires_at_exp = auth_util::update_expires_at(
            transaction,
            *token_hash_exp,
            token_util::TOKEN_TTL
        );
        if(!update_expires_at_exp){
            return std::unexpected(update_expires_at_exp.error());
        }
        if(!update_expires_at_exp.value()){
            return false;
        }

        transaction.commit();
        return true;
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<bool, error_code> auth_service::revoke_token(
    db_connection& connection_value,
    std::string_view token
){
    if(!connection_value.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(token.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto token_hash_exp = crypto_util::sha512_hex(token);
    if(!token_hash_exp){
        return std::unexpected(token_hash_exp.error());
    }

    try{
        pqxx::work transaction(connection_value.connection());
        const auto revoke_token_exp = auth_util::revoke_token(transaction, *token_hash_exp);
        if(!revoke_token_exp){
            return std::unexpected(revoke_token_exp.error());
        }
        if(!revoke_token_exp.value()){
            return false;
        }

        transaction.commit();
        return true;
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}
