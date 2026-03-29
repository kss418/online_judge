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

std::expected<std::optional<user_dto::submission_ban>, error_code>
user_service::create_submission_ban(
    db_connection& connection,
    std::int64_t user_id,
    std::int32_t duration_minutes
){
    if(user_id <= 0 || duration_minutes <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return db_service_util::with_retry_write_transaction(
        connection,
        [&](pqxx::work& transaction)
            -> std::expected<std::optional<user_dto::submission_ban>, error_code> {
            const auto create_submission_ban_exp = user_repository::create_submission_ban(
                transaction,
                user_id,
                duration_minutes
            );
            if(!create_submission_ban_exp){
                return std::unexpected(create_submission_ban_exp.error());
            }
            if(!create_submission_ban_exp->has_value()){
                return std::optional<user_dto::submission_ban>{std::nullopt};
            }

            user_dto::submission_ban submission_ban_value;
            submission_ban_value.user_id = user_id;
            submission_ban_value.duration_minutes = duration_minutes;
            submission_ban_value.submission_banned_until =
                std::move(create_submission_ban_exp->value());
            return submission_ban_value;
        }
    );
}

std::expected<bool, error_code> user_service::update_submission_banned_until(
    db_connection& connection,
    std::int64_t user_id,
    std::string_view submission_banned_until
){
    if(user_id <= 0 || submission_banned_until.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return db_service_util::with_retry_write_transaction(
        connection,
        [&](pqxx::work& transaction) -> std::expected<bool, error_code> {
            const auto update_submission_banned_until_exp =
                user_repository::update_submission_banned_until(
                    transaction,
                    user_id,
                    submission_banned_until
                );
            if(!update_submission_banned_until_exp){
                return std::unexpected(update_submission_banned_until_exp.error());
            }

            return *update_submission_banned_until_exp;
        }
    );
}

std::expected<bool, error_code> user_service::clear_submission_banned_until(
    db_connection& connection,
    std::int64_t user_id
){
    if(user_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return db_service_util::with_retry_write_transaction(
        connection,
        [&](pqxx::work& transaction) -> std::expected<bool, error_code> {
            const auto clear_submission_banned_until_exp =
                user_repository::clear_submission_banned_until(
                    transaction,
                    user_id
                );
            if(!clear_submission_banned_until_exp){
                return std::unexpected(clear_submission_banned_until_exp.error());
            }

            return *clear_submission_banned_until_exp;
        }
    );
}
