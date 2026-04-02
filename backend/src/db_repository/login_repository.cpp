#include "db_repository/login_repository.hpp"
#include "db_repository/db_repository.hpp"

#include <pqxx/pqxx>

std::expected<std::int64_t, repository_error> login_repository::create_user(
    pqxx::transaction_base& transaction,
    const auth_dto::hashed_sign_up_request& sign_up_request_value
){
    if(!auth_dto::is_valid(sign_up_request_value)){
        return std::unexpected(db_repository::invalid_input_error());
    }

    const auto create_user_result = transaction.exec(
        "WITH new_user_info AS ("
        "    INSERT INTO user_info DEFAULT VALUES "
        "    RETURNING user_id"
        ") "
        "INSERT INTO users(user_id, user_login_id, user_password_hash) "
        "SELECT user_id, $1, $2 "
        "FROM new_user_info "
        "RETURNING user_id",
        pqxx::params{
            sign_up_request_value.user_login_id,
            sign_up_request_value.password_hash
        }
    );

    return create_user_result[0][0].as<std::int64_t>();
}

std::expected<bool, repository_error> login_repository::verify_user(
    pqxx::transaction_base& transaction,
    const auth_dto::hashed_credentials& credentials_value
){
    if(!auth_dto::is_valid(credentials_value)){
        return std::unexpected(db_repository::invalid_input_error());
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

std::expected<std::optional<auth_dto::identity>, repository_error> login_repository::get_login_identity(
    pqxx::transaction_base& transaction,
    const auth_dto::hashed_credentials& credentials_value
){
    if(!auth_dto::is_valid(credentials_value)){
        return std::unexpected(db_repository::invalid_input_error());
    }

    const auto login_identity_result = transaction.exec(
        "SELECT user_id, permission_level, COALESCE(user_login_id, '') "
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
    identity_value.permission_level = login_identity_result[0][1].as<std::int32_t>();
    identity_value.user_login_id = login_identity_result[0][2].as<std::string>();
    return identity_value;
}
