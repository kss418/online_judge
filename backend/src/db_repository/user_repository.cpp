#include "db_repository/user_repository.hpp"

#include <pqxx/pqxx>

std::expected<std::optional<user_dto::summary>, error_code> user_repository::get_summary(
    pqxx::transaction_base& transaction,
    std::int64_t user_id
){
    if(user_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto user_summary_result = transaction.exec(
        "SELECT "
        "user_id, "
        "COALESCE(user_login_id, ''), "
        "created_at::text "
        "FROM users "
        "WHERE user_id = $1",
        pqxx::params{user_id}
    );

    if(user_summary_result.empty()){
        return std::nullopt;
    }

    return user_dto::make_summary_from_row(user_summary_result[0]);
}

std::expected<std::optional<user_dto::summary>, error_code>
user_repository::get_summary_by_login_id(
    pqxx::transaction_base& transaction,
    std::string_view user_login_id
){
    if(user_login_id.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto user_summary_result = transaction.exec(
        "SELECT "
        "user_id, "
        "COALESCE(user_login_id, ''), "
        "created_at::text "
        "FROM users "
        "WHERE user_login_id = $1",
        pqxx::params{user_login_id}
    );

    if(user_summary_result.empty()){
        return std::nullopt;
    }

    return user_dto::make_summary_from_row(user_summary_result[0]);
}
