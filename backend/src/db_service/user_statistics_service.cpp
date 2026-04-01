#include "db_service/user_statistics_service.hpp"

#include "db_repository/user_statistics_repository.hpp"
#include "db_service/db_service_util.hpp"

std::expected<user_statistics_dto::submission_statistics, error_code>
user_statistics_service::get_submission_statistics(
    db_connection& connection,
    std::int64_t user_id
){
    return db_service_util::with_retry_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<user_statistics_dto::submission_statistics, error_code> {
            return db_service_util::map_repository_error_to_http_error(
                user_statistics_repository::get_submission_statistics(
                    transaction,
                    user_id
                )
            );
        }
    );
}

std::expected<void, error_code> user_statistics_service::create_user_submission_statistics(
    db_connection& connection,
    std::int64_t user_id
){
    return db_service_util::with_retry_write_transaction(
        connection,
        db_service_util::DB_TRANSACTION_ATTEMPT_COUNT,
        [&](pqxx::work& transaction) -> std::expected<void, error_code> {
            return db_service_util::map_repository_error_to_http_error(
                user_statistics_repository::create_user_submission_statistics(
                    transaction,
                    user_id
                )
            );
        }
    );
}

std::expected<void, error_code> user_statistics_service::ensure_user_submission_statistics(
    db_connection& connection,
    std::int64_t user_id
){
    return db_service_util::with_retry_write_transaction(
        connection,
        db_service_util::DB_TRANSACTION_ATTEMPT_COUNT,
        [&](pqxx::work& transaction) -> std::expected<void, error_code> {
            return db_service_util::map_repository_error_to_http_error(
                user_statistics_repository::ensure_user_submission_statistics(
                    transaction,
                    user_id
                )
            );
        }
    );
}

std::expected<void, error_code> user_statistics_service::record_submission_created(
    db_connection& connection,
    std::int64_t user_id
){
    if(user_id <= 0){
        return std::unexpected(error_code::create(http_error::validation_error));
    }

    return db_service_util::with_retry_write_transaction(
        connection,
        db_service_util::DB_TRANSACTION_ATTEMPT_COUNT,
        [&](pqxx::work& transaction) -> std::expected<void, error_code> {
            const auto ensure_statistics_exp =
                db_service_util::map_repository_error_to_http_error(
                    user_statistics_repository::ensure_user_submission_statistics(
                        transaction,
                        user_id
                    )
                );
            if(!ensure_statistics_exp){
                return std::unexpected(ensure_statistics_exp.error());
            }

            const auto increase_submission_count_exp =
                db_service_util::map_repository_error_to_http_error(
                    user_statistics_repository::increase_submission_count(
                        transaction,
                        user_id
                    )
                );
            if(!increase_submission_count_exp){
                return std::unexpected(increase_submission_count_exp.error());
            }

            const auto increase_queued_count_exp =
                db_service_util::map_repository_error_to_http_error(
                    user_statistics_repository::increase_status_count(
                        transaction,
                        user_id,
                        submission_status::queued
                    )
                );
            if(!increase_queued_count_exp){
                return std::unexpected(increase_queued_count_exp.error());
            }

            return db_service_util::map_repository_error_to_http_error(
                user_statistics_repository::touch_last_submission_at(
                    transaction,
                    user_id
                )
            );
        }
    );
}

std::expected<void, error_code> user_statistics_service::record_submission_status_transition(
    db_connection& connection,
    std::int64_t user_id,
    std::optional<submission_status> from_status_opt,
    submission_status to_status
){
    if(user_id <= 0){
        return std::unexpected(error_code::create(http_error::validation_error));
    }

    return db_service_util::with_retry_write_transaction(
        connection,
        db_service_util::DB_TRANSACTION_ATTEMPT_COUNT,
        [&](pqxx::work& transaction) -> std::expected<void, error_code> {
            const auto ensure_statistics_exp =
                db_service_util::map_repository_error_to_http_error(
                    user_statistics_repository::ensure_user_submission_statistics(
                        transaction,
                        user_id
                    )
                );
            if(!ensure_statistics_exp){
                return std::unexpected(ensure_statistics_exp.error());
            }

            if(from_status_opt && *from_status_opt != to_status){
                const auto decrease_from_status_exp =
                    db_service_util::map_repository_error_to_http_error(
                        user_statistics_repository::decrease_status_count(
                            transaction,
                            user_id,
                            *from_status_opt
                        )
                    );
                if(!decrease_from_status_exp){
                    return std::unexpected(decrease_from_status_exp.error());
                }
            }

            if(!from_status_opt || *from_status_opt != to_status){
                const auto increase_to_status_exp =
                    db_service_util::map_repository_error_to_http_error(
                        user_statistics_repository::increase_status_count(
                            transaction,
                            user_id,
                            to_status
                        )
                    );
                if(!increase_to_status_exp){
                    return std::unexpected(increase_to_status_exp.error());
                }
            }

            if(to_status == submission_status::accepted){
                const auto touch_last_accepted_at_exp =
                    db_service_util::map_repository_error_to_http_error(
                        user_statistics_repository::touch_last_accepted_at(
                            transaction,
                            user_id
                        )
                    );
                if(!touch_last_accepted_at_exp){
                    return std::unexpected(touch_last_accepted_at_exp.error());
                }
            }

            return {};
        }
    );
}
