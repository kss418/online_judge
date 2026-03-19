#include "db_service/login_service.hpp"
#include "common/password_util.hpp"
#include "common/token_util.hpp"
#include "db_util/auth_util.hpp"
#include "db_util/login_util.hpp"

#include <pqxx/pqxx>

std::expected<auth_dto::session, error_code> login_service::sign_up(
    db_connection& connection_value,
    const auth_dto::credentials& credentials_value
){
    if(!connection_value.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(
        credentials_value.user_login_id.empty() ||
        credentials_value.raw_password.empty()
    ){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto user_password_hash_exp = password_util::get_password_hash(
        credentials_value.raw_password
    );
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
            credentials_value.user_login_id,
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

        auth_dto::session session_value;
        session_value.user_id = *user_id_exp;
        session_value.is_admin = false;
        session_value.token = issued_token_exp->token;

        transaction.commit();
        return session_value;
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<std::optional<auth_dto::session>, error_code> login_service::login(
    db_connection& connection_value,
    const auth_dto::credentials& credentials_value
){
    if(!connection_value.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(
        credentials_value.user_login_id.empty() ||
        credentials_value.raw_password.empty()
    ){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto user_password_hash_exp = password_util::get_password_hash(
        credentials_value.raw_password
    );
    if(!user_password_hash_exp){
        return std::unexpected(user_password_hash_exp.error());
    }

    try{
        pqxx::work transaction(connection_value.connection());
        const auto login_identity_exp = login_util::get_login_identity(
            transaction,
            credentials_value.user_login_id,
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

        auth_dto::session session_value;
        session_value.user_id = login_identity_exp->value().user_id;
        session_value.is_admin = login_identity_exp->value().is_admin;
        session_value.token = issued_token_exp->token;

        transaction.commit();
        return session_value;
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}
