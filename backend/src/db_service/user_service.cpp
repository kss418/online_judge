#include "db_service/user_service.hpp"

#include "db_repository/user_repository.hpp"
#include "db_service/db_service_util.hpp"

std::expected<user_dto::list, error_code> user_service::get_public_list(
    db_connection& connection,
    const user_dto::list_filter& filter_value
){
    return db_service_util::with_retry_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<user_dto::list, error_code> {
            return user_repository::get_public_list(
                transaction,
                filter_value
            );
        }
    );
}

std::expected<std::optional<user_dto::summary>, error_code> user_service::get_summary(
    db_connection& connection,
    std::int64_t user_id
){
    if(user_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return db_service_util::with_retry_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<std::optional<user_dto::summary>, error_code> {
            return user_repository::get_summary(
                transaction,
                user_id
            );
        }
    );
}

std::expected<std::optional<user_dto::summary>, error_code>
user_service::get_summary_by_login_id(
    db_connection& connection,
    std::string_view user_login_id
){
    if(user_login_id.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return db_service_util::with_retry_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<std::optional<user_dto::summary>, error_code> {
            return user_repository::get_summary_by_login_id(
                transaction,
                user_login_id
            );
        }
    );
}
