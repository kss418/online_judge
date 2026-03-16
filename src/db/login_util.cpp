#include "db/login_util.hpp"

#include <pqxx/pqxx>

std::expected<std::int64_t, error_code> login_util::create_user(
    pqxx::transaction_base& transaction,
    std::string_view user_login_id,
    std::string_view user_password_hash
){
    if(user_login_id.empty() || user_password_hash.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto create_user_result = transaction.exec(
        "INSERT INTO users(user_login_id, user_password_hash) "
        "VALUES($1, $2) "
        "ON CONFLICT DO NOTHING "
        "RETURNING user_id",
        pqxx::params{user_login_id, user_password_hash}
    );

    if(create_user_result.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return create_user_result[0][0].as<std::int64_t>();
}

std::expected<bool, error_code> login_util::verify_user(
    pqxx::transaction_base& transaction,
    std::string_view user_login_id,
    std::string_view user_password_hash
){
    if(user_login_id.empty() || user_password_hash.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto verify_result = transaction.exec(
        "SELECT 1 "
        "FROM users "
        "WHERE "
        "user_login_id = $1 AND "
        "user_password_hash = $2 "
        "LIMIT 1",
        pqxx::params{user_login_id, user_password_hash}
    );

    return !verify_result.empty();
}
