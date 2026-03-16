#include "db/auth_service.hpp"
#include "db/auth_util.hpp"
#include "common/token_util.hpp"

#include <pqxx/pqxx>

#include <chrono>
#include <utility>

auth_service::auth_service(db_connection connection) :
    db_service_base<auth_service>(std::move(connection)){}

std::expected<std::string, error_code> auth_service::issue_token(std::int64_t user_id){
    if(!is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    
    if(user_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    for(int issue_attempt = 0; issue_attempt < ISSUE_MAX_ATTEMPTS; ++issue_attempt){
        auto issued_token_exp = token_util::issue_token();
        if(!issued_token_exp){
            return std::unexpected(issued_token_exp.error());
        }

        try{
            pqxx::work transaction(connection());
            const auto insert_token_exp = auth_util::insert_token(
                transaction,
                user_id,
                issued_token_exp->token_hash,
                TOKEN_TTL
            );
            if(!insert_token_exp){
                return std::unexpected(insert_token_exp.error());
            }

            transaction.commit();
            return std::move(issued_token_exp->token);
        }
        catch(const pqxx::unique_violation&){
            if(issue_attempt + 1 == ISSUE_MAX_ATTEMPTS){
                return std::unexpected(error_code::create(psql_error::unique_violation));
            }
        }
        catch(const std::exception& exception){
            return std::unexpected(error_code::map_psql_error_code(exception));
        }
    }

    return std::unexpected(error_code::create(errno_error::unknown_error));
}

std::expected<std::optional<auth_identity>, error_code> auth_service::auth_token(
    std::string_view token
){
    if(!is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(token.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto token_hash_exp = token_util::hash_token(token);
    if(!token_hash_exp){
        return std::unexpected(token_hash_exp.error());
    }

    try{
        pqxx::work transaction(connection());
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
        auth_identity auth_identity_value;
        auth_identity_value.user_id = token_identity_value.user_id;
        auth_identity_value.is_admin = token_identity_value.is_admin;

        transaction.commit();
        return auth_identity_value;
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<bool, error_code> auth_service::renew_token(std::string_view token){
    if(!is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(token.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto token_hash_exp = token_util::hash_token(token);
    if(!token_hash_exp){
        return std::unexpected(token_hash_exp.error());
    }

    try{
        pqxx::work transaction(connection());
        const auto update_expires_at_exp = auth_util::update_expires_at(
            transaction,
            *token_hash_exp,
            TOKEN_TTL
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

std::expected<bool, error_code> auth_service::revoke_token(std::string_view token){
    if(!is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(token.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto token_hash_exp = token_util::hash_token(token);
    if(!token_hash_exp){
        return std::unexpected(token_hash_exp.error());
    }

    try{
        pqxx::work transaction(connection());
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
