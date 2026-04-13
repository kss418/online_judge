#include "db_service/submission_judge_service.hpp"

#include "db_service/db_service_util.hpp"
#include "db_repository/problem_statistics_repository.hpp"
#include "db_repository/submission_repository.hpp"

#include <utility>

std::expected<void, service_error> submission_judge_service::mark_judging(
    db_connection& connection,
    const submission_domain_dto::leased_submission& leased_submission_value
){
    const submission_internal_dto::status_update status_update_value =
        submission_internal_dto::make_status_update(
            leased_submission_value,
            submission_status::judging
        );
    return db_service_util::with_retry_service_write_transaction(
        connection,
        [&](pqxx::work& transaction) -> std::expected<void, service_error> {
            return submission_repository::update_submission_status(
                transaction,
                status_update_value
            );
        }
    );
}

std::expected<std::optional<submission_domain_dto::leased_submission>, service_error>
submission_judge_service::lease_submission(
    db_connection& connection,
    const submission_internal_dto::lease_request& lease_request_value
){
    return db_service_util::with_retry_service_write_transaction(
        connection,
        [&](pqxx::work& transaction)
            -> std::expected<std::optional<submission_domain_dto::leased_submission>, service_error> {
            auto lease_submission_exp = submission_repository::lease_submission(
                transaction,
                lease_request_value
            );
            if(!lease_submission_exp){
                return std::unexpected(lease_submission_exp.error());
            }

            return std::move(*lease_submission_exp);
        }
    );
}

std::expected<void, service_error> submission_judge_service::requeue_submission_immediately(
    db_connection& connection,
    const submission_domain_dto::leased_submission& leased_submission_value,
    std::optional<std::string> reason_opt
){
    const std::optional<std::string> queued_reason_opt = std::move(reason_opt);
    return db_service_util::with_retry_service_write_transaction(
        connection,
        [&](pqxx::work& transaction) -> std::expected<void, service_error> {
            const submission_internal_dto::status_update status_update_value =
                submission_internal_dto::make_status_update(
                    leased_submission_value,
                    submission_status::queued,
                    queued_reason_opt
                );
            const auto update_submission_status_exp =
                submission_repository::update_submission_status(
                    transaction,
                    status_update_value
                );
            if(!update_submission_status_exp){
                return std::unexpected(update_submission_status_exp.error());
            }

            return submission_repository::release_submission_lease(
                transaction,
                leased_submission_value
            );
        }
    );
}

std::expected<void, service_error> submission_judge_service::finalize_submission(
    db_connection& connection,
    const submission_internal_dto::finalize_request& finalize_request_value
){
    return db_service_util::with_retry_service_write_transaction(
        connection,
        db_service_util::DB_TRANSACTION_ATTEMPT_COUNT,
        [&](pqxx::work& transaction) -> std::expected<void, service_error> {
            const auto finalize_submission_exp = submission_repository::finalize_submission(
                transaction,
                finalize_request_value
            );
            if(!finalize_submission_exp){
                return std::unexpected(finalize_submission_exp.error());
            }

            problem_dto::reference problem_reference_value{
                finalize_submission_exp->problem_id
            };
            if(finalize_submission_exp->should_increase_accepted_count){
                const auto increase_accepted_count_exp =
                    problem_statistics_repository::increase_accepted_count(
                        transaction,
                        problem_reference_value
                    );
                if(!increase_accepted_count_exp){
                    return std::unexpected(increase_accepted_count_exp.error());
                }
            }

            return {};
        }
    );
}
