#include "db_service/user_service.hpp"

#include "db_repository/user_repository.hpp"
#include "db_service/db_service_util.hpp"

std::expected<user_dto::list, service_error> user_service::get_public_list(
    db_connection& connection,
    const user_dto::list_filter& filter_value
){
    return db_service_util::with_retry_service_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<user_dto::list, service_error> {
            return user_repository::get_public_list(
                transaction,
                filter_value
            );
        }
    );
}

std::expected<void, service_error> user_service::ensure_user_exists(
    db_connection& connection,
    std::int64_t user_id
){
    if(user_id <= 0){
        return std::unexpected(service_error::validation_error);
    }

    return db_service_util::with_retry_service_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<void, service_error> {
            const auto exists_exp = user_repository::exists_user(
                transaction,
                user_id
            );
            if(!exists_exp){
                return std::unexpected(exists_exp.error());
            }
            if(!*exists_exp){
                return std::unexpected(service_error::not_found);
            }

            return {};
        }
    );
}

std::expected<user_dto::summary, service_error> user_service::get_summary(
    db_connection& connection,
    std::int64_t user_id
){
    if(user_id <= 0){
        return std::unexpected(service_error::validation_error);
    }

    return db_service_util::with_retry_service_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<user_dto::summary, service_error> {
            return user_repository::get_summary(transaction, user_id);
        }
    );
}

std::expected<user_dto::summary, service_error>
user_service::get_summary_by_login_id(
    db_connection& connection,
    std::string_view user_login_id
){
    if(user_login_id.empty()){
        return std::unexpected(service_error::validation_error);
    }

    return db_service_util::with_retry_service_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<user_dto::summary, service_error> {
            return user_repository::get_summary_by_login_id(
                transaction,
                user_login_id
            );
        }
    );
}

std::expected<user_dto::submission_ban, service_error>
user_service::create_submission_ban(
    db_connection& connection,
    std::int64_t user_id,
    std::int32_t duration_minutes
){
    if(user_id <= 0 || duration_minutes <= 0){
        return std::unexpected(service_error::validation_error);
    }

    return db_service_util::with_retry_service_write_transaction(
        connection,
        [&](pqxx::work& transaction)
            -> std::expected<user_dto::submission_ban, service_error> {
            const auto create_submission_ban_exp =
                user_repository::create_submission_ban(
                    transaction,
                    user_id,
                    duration_minutes
                );
            if(!create_submission_ban_exp){
                return std::unexpected(create_submission_ban_exp.error());
            }

            user_dto::submission_ban submission_ban_value;
            submission_ban_value.user_id = user_id;
            submission_ban_value.duration_minutes = duration_minutes;
            submission_ban_value.submission_banned_until =
                std::move(*create_submission_ban_exp);
            return submission_ban_value;
        }
    );
}

std::expected<user_dto::submission_ban_status, service_error>
user_service::get_submission_ban_status(
    db_connection& connection,
    std::int64_t user_id
){
    if(user_id <= 0){
        return std::unexpected(service_error::validation_error);
    }

    return db_service_util::with_retry_service_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<user_dto::submission_ban_status, service_error> {
            const auto submission_ban_status_exp = user_repository::get_submission_ban_status(
                transaction,
                user_id
            );
            if(!submission_ban_status_exp){
                return std::unexpected(submission_ban_status_exp.error());
            }

            return std::move(*submission_ban_status_exp);
        }
    );
}

std::expected<void, service_error> user_service::clear_submission_banned_until(
    db_connection& connection,
    std::int64_t user_id
){
    if(user_id <= 0){
        return std::unexpected(service_error::validation_error);
    }

    return db_service_util::with_retry_service_write_transaction(
        connection,
        [&](pqxx::work& transaction) -> std::expected<void, service_error> {
            const auto clear_submission_banned_until_exp =
                user_repository::clear_submission_banned_until(
                    transaction,
                    user_id
                );
            if(!clear_submission_banned_until_exp){
                return std::unexpected(clear_submission_banned_until_exp.error());
            }

            return {};
        }
    );
}
