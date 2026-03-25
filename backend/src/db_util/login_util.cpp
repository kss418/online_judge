#include "db_util/login_util.hpp"

#include <pqxx/pqxx>

std::expected<std::int64_t, error_code> login_util::create_user(
    pqxx::transaction_base& transaction,
    const auth_dto::hashed_sign_up_request& sign_up_request_value
){
    if(
        sign_up_request_value.user_name.empty() ||
        sign_up_request_value.user_login_id.empty() ||
        sign_up_request_value.password_hash.empty()
    ){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto create_user_result = transaction.exec(
        "INSERT INTO users(user_name, user_login_id, user_password_hash) "
        "VALUES($1, $2, $3) "
        "RETURNING user_id",
        pqxx::params{
            sign_up_request_value.user_name,
            sign_up_request_value.user_login_id,
            sign_up_request_value.password_hash
        }
    );

    return create_user_result[0][0].as<std::int64_t>();
}

std::expected<bool, error_code> login_util::verify_user(
    pqxx::transaction_base& transaction,
    const auth_dto::hashed_credentials& credentials_value
){
    if(
        credentials_value.user_login_id.empty() ||
        credentials_value.password_hash.empty()
    ){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto verify_result = transaction.exec(
        "SELECT 1 "
        "FROM users "
        "WHERE "
        "user_login_id = $1 AND "
        "user_password_hash = $2 "
        "LIMIT 1",
        pqxx::params{credentials_value.user_login_id, credentials_value.password_hash}
    );

    return !verify_result.empty();
}

std::expected<std::optional<auth_dto::identity>, error_code> login_util::get_login_identity(
    pqxx::transaction_base& transaction,
    const auth_dto::hashed_credentials& credentials_value
){
    if(
        credentials_value.user_login_id.empty() ||
        credentials_value.password_hash.empty()
    ){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto login_identity_result = transaction.exec(
        "SELECT user_id, is_admin, user_name "
        "FROM users "
        "WHERE "
        "user_login_id = $1 AND "
        "user_password_hash = $2 "
        "LIMIT 1",
        pqxx::params{credentials_value.user_login_id, credentials_value.password_hash}
    );

    if(login_identity_result.empty()){
        return std::nullopt;
    }

    auth_dto::identity identity_value;
    identity_value.user_id = login_identity_result[0][0].as<std::int64_t>();
    identity_value.is_admin = login_identity_result[0][1].as<bool>();
    identity_value.user_name = login_identity_result[0][2].as<std::string>();
    return identity_value;
}
