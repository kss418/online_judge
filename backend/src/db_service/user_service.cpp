#include "db_service/user_service.hpp"

#include "db_repository/user_repository.hpp"
#include "db_service/db_service_util.hpp"

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
