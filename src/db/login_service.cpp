#include "db/login_service.hpp"
#include "common/password_util.hpp"
#include "common/token_util.hpp"
#include "db/auth_util.hpp"
#include "db/login_util.hpp"

#include <pqxx/pqxx>

std::expected<login_service::login_session, error_code> login_service::sign_up(
    db_connection& connection_value,
    std::string_view user_login_id,
    std::string_view raw_password
){
    if(!connection_value.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(user_login_id.empty() || raw_password.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto user_password_hash_exp = password_util::get_password_hash(raw_password);
    if(!user_password_hash_exp){
        return std::unexpected(user_password_hash_exp.error());
    }

    const auto issued_token_exp = token_util::issue_token();
    if(!issued_token_exp){
        return std::unexpected(issued_token_exp.error());
    }

    try{
        pqxx::work transaction(connection_value.connection());
        const auto user_id_exp = login_util::create_user(
            transaction,
            user_login_id,
            *user_password_hash_exp
        );
        if(!user_id_exp){
            return std::unexpected(user_id_exp.error());
        }

        const auto insert_token_exp = auth_util::insert_token(
            transaction,
            *user_id_exp,
            issued_token_exp->token_hash,
            token_util::TOKEN_TTL
        );
        if(!insert_token_exp){
            return std::unexpected(insert_token_exp.error());
        }

        login_session login_session_value;
        login_session_value.user_id = *user_id_exp;
        login_session_value.is_admin = false;
        login_session_value.token = issued_token_exp->token;

        transaction.commit();
        return login_session_value;
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<std::optional<login_service::login_session>, error_code> login_service::login(
    db_connection& connection_value,
    std::string_view user_login_id,
    std::string_view raw_password
){
    if(!connection_value.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(user_login_id.empty() || raw_password.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto user_password_hash_exp = password_util::get_password_hash(raw_password);
    if(!user_password_hash_exp){
        return std::unexpected(user_password_hash_exp.error());
    }

    try{
        pqxx::work transaction(connection_value.connection());
        const auto login_identity_exp = login_util::get_login_identity(
            transaction,
            user_login_id,
            *user_password_hash_exp
        );
        if(!login_identity_exp){
            return std::unexpected(login_identity_exp.error());
        }
        if(!login_identity_exp->has_value()){
            return std::nullopt;
        }

        const auto issued_token_exp = token_util::issue_token();
        if(!issued_token_exp){
            return std::unexpected(issued_token_exp.error());
        }

        const auto insert_token_exp = auth_util::insert_token(
            transaction,
            login_identity_exp->value().user_id,
            issued_token_exp->token_hash,
            token_util::TOKEN_TTL
        );
        if(!insert_token_exp){
            return std::unexpected(insert_token_exp.error());
        }

        login_session login_session_value;
        login_session_value.user_id = login_identity_exp->value().user_id;
        login_session_value.is_admin = login_identity_exp->value().is_admin;
        login_session_value.token = issued_token_exp->token;

        transaction.commit();
        return login_session_value;
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}
