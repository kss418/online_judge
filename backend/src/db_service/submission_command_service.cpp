#include "db_service/submission_command_service.hpp"

#include "db_service/db_service_util.hpp"
#include "db_repository/problem_statistics_repository.hpp"
#include "db_repository/submission_repository.hpp"
#include "db_repository/user_problem_summary_repository.hpp"
#include "db_repository/user_repository.hpp"

std::expected<submission_response_dto::queued_response, service_error>
submission_command_service::create_submission(
    db_connection& connection,
    const submission_internal_dto::create_submission_command& create_request_value
){
    problem_dto::reference problem_reference_value{create_request_value.problem_id};

    return db_service_util::with_retry_service_write_transaction(
        connection,
        [&](pqxx::work& transaction)
            -> std::expected<submission_response_dto::queued_response, service_error> {
            const auto active_submission_ban_exp =
                user_repository::get_active_submission_banned_until(
                    transaction,
                    create_request_value.user_id
                );
            if(!active_submission_ban_exp){
                return std::unexpected(active_submission_ban_exp.error());
            }
            if(active_submission_ban_exp->has_value()){
                return std::unexpected(service_error::forbidden);
            }

            const auto create_submission_exp = submission_repository::create_submission(
                transaction,
                create_request_value
            );
            if(!create_submission_exp){
                return std::unexpected(create_submission_exp.error());
            }

            const auto increase_user_problem_submission_count_exp =
                user_problem_summary_repository::increase_submission_count(
                    transaction,
                    create_request_value.user_id,
                    create_request_value.problem_id
                );
            if(!increase_user_problem_submission_count_exp){
                return std::unexpected(
                    increase_user_problem_submission_count_exp.error()
                );
            }

            const auto enqueue_submission_exp = submission_repository::enqueue_submission(
                transaction,
                create_submission_exp->submission_id,
                submission_repository::NORMAL_SUBMISSION_QUEUE_PRIORITY
            );
            if(!enqueue_submission_exp){
                return std::unexpected(enqueue_submission_exp.error());
            }

            const auto increase_submission_count_exp =
                problem_statistics_repository::increase_submission_count(
                    transaction,
                    problem_reference_value
                );
            if(!increase_submission_count_exp){
                return std::unexpected(increase_submission_count_exp.error());
            }

            return *create_submission_exp;
        }
    );
}

std::expected<submission_response_dto::queued_response, service_error>
submission_command_service::rejudge_submission(
    db_connection& connection,
    std::int64_t submission_id
){
    return db_service_util::with_retry_service_write_transaction(
        connection,
        [&](pqxx::work& transaction)
            -> std::expected<submission_response_dto::queued_response, service_error> {
            return submission_repository::rejudge_submission(
                transaction,
                submission_id
            );
        }
    );
}

std::expected<void, service_error> submission_command_service::rejudge_problem(
    db_connection& connection,
    std::int64_t problem_id
){
    return db_service_util::with_retry_service_write_transaction(
        connection,
        [&](pqxx::work& transaction) -> std::expected<void, service_error> {
            const auto submission_values_exp =
                submission_repository::get_wa_or_ac_submissions(
                    transaction,
                    problem_id
                );
            if(!submission_values_exp){
                return std::unexpected(submission_values_exp.error());
            }

            for(const auto& submission_value : *submission_values_exp){
                const auto rejudge_submission_exp =
                    submission_repository::rejudge_submission(
                        transaction,
                        submission_value.submission_id
                    );
                if(!rejudge_submission_exp){
                    return std::unexpected(rejudge_submission_exp.error());
                }
            }

            return {};
        }
    );
}
