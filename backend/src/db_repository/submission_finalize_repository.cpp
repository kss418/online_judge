#include "db_repository/submission_finalize_repository.hpp"

#include "common/submission_status.hpp"
#include "db_repository/problem_statistics_repository.hpp"
#include "db_repository/submission_queue_repository.hpp"
#include "db_repository/user_problem_summary_repository.hpp"
#include "row_mapper/submission_row_mapper.hpp"

#include <pqxx/pqxx>

namespace{
    struct locked_submission_context{
        std::int64_t user_id = 0;
        std::int64_t problem_id = 0;
        std::int32_t problem_version = 0;
        submission_status status = submission_status::queued;
    };

    std::expected<locked_submission_context, repository_error>
    get_locked_leased_submission_context(
        pqxx::transaction_base& transaction,
        std::int64_t submission_id,
        std::int32_t attempt_no,
        const std::string& lease_token
    ){
        if(submission_id <= 0 || attempt_no <= 0 || lease_token.empty()){
            return std::unexpected(repository_error::invalid_reference);
        }

        const auto submission_result = transaction.exec(
            "SELECT "
            "submission_table.user_id, "
            "submission_table.problem_id, "
            "submission_table.problem_version, "
            "submission_table.status::text "
            "FROM submissions submission_table "
            "JOIN submission_queue queue_table "
            "ON queue_table.submission_id = submission_table.submission_id "
            "WHERE "
            "submission_table.submission_id = $1 AND "
            "queue_table.attempt_no = $2 AND "
            "queue_table.lease_token = $3 AND "
            "queue_table.leased_until IS NOT NULL AND "
            "queue_table.leased_until > NOW() "
            "FOR UPDATE OF submission_table, queue_table",
            pqxx::params{submission_id, attempt_no, lease_token}
        );
        if(submission_result.empty()){
            return std::unexpected(
                repository_error{
                    repository_error_code::conflict,
                    "submission lease is stale"
                }
            );
        }

        const auto submission_status_exp = submission_row_mapper::map_submission_status_row(
            submission_result[0],
            3
        );
        if(!submission_status_exp){
            return std::unexpected(submission_status_exp.error());
        }

        locked_submission_context context_value;
        context_value.user_id = submission_result[0][0].as<std::int64_t>();
        context_value.problem_id = submission_result[0][1].as<std::int64_t>();
        context_value.problem_version = submission_result[0][2].as<std::int32_t>();
        context_value.status = *submission_status_exp;
        return context_value;
    }

    std::expected<locked_submission_context, repository_error> get_locked_submission_context(
        pqxx::transaction_base& transaction,
        std::int64_t submission_id
    ){
        if(submission_id <= 0){
            return std::unexpected(repository_error::invalid_reference);
        }

        const auto submission_result = transaction.exec(
            "SELECT user_id, problem_id, problem_version, status::text "
            "FROM submissions "
            "WHERE submission_id = $1 "
            "FOR UPDATE",
            pqxx::params{submission_id}
        );
        if(submission_result.empty()){
            return std::unexpected(repository_error::not_found);
        }

        const auto submission_status_exp = submission_row_mapper::map_submission_status_row(
            submission_result[0],
            3
        );
        if(!submission_status_exp){
            return std::unexpected(submission_status_exp.error());
        }

        locked_submission_context context_value;
        context_value.user_id = submission_result[0][0].as<std::int64_t>();
        context_value.problem_id = submission_result[0][1].as<std::int64_t>();
        context_value.problem_version = submission_result[0][2].as<std::int32_t>();
        context_value.status = *submission_status_exp;
        return context_value;
    }

    std::expected<void, repository_error> persist_submission_status_transition(
        pqxx::transaction_base& transaction,
        std::int64_t submission_id,
        submission_status from_status,
        submission_status to_status,
        const std::optional<std::string>& reason_opt
    ){
        if(submission_id <= 0){
            return std::unexpected(repository_error::invalid_reference);
        }

        transaction.exec(
            "UPDATE submissions "
            "SET status = $2::submission_status, updated_at = NOW() "
            "WHERE submission_id = $1",
            pqxx::params{submission_id, to_string(to_status)}
        );

        transaction.exec(
            "INSERT INTO submission_status_history(submission_id, from_status, to_status, reason) "
            "VALUES($1, $2::submission_status, $3::submission_status, $4)",
            pqxx::params{
                submission_id,
                to_string(from_status),
                to_string(to_status),
                reason_opt
            }
        );

        return {};
    }

    std::expected<void, repository_error> clear_submission_result(
        pqxx::transaction_base& transaction,
        std::int64_t submission_id
    ){
        if(submission_id <= 0){
            return std::unexpected(repository_error::invalid_reference);
        }

        const auto update_result = transaction.exec(
            "UPDATE submissions "
            "SET "
            "score = NULL, "
            "compile_output = NULL, "
            "judge_output = NULL, "
            "elapsed_ms = NULL, "
            "max_rss_kb = NULL, "
            "updated_at = NOW() "
            "WHERE submission_id = $1",
            pqxx::params{submission_id}
        );

        if(update_result.affected_rows() == 0){
            return std::unexpected(repository_error::not_found);
        }

        return {};
    }
}

std::expected<void, repository_error> submission_finalize_repository::update_submission_status(
    pqxx::transaction_base& transaction,
    const submission_internal_dto::status_update& status_update_value
){
    if(!submission_internal_dto::is_valid(status_update_value)){
        return std::unexpected(repository_error::invalid_reference);
    }

    const std::int64_t submission_id = status_update_value.submission_id;
    const auto locked_submission_exp = get_locked_leased_submission_context(
        transaction,
        submission_id,
        status_update_value.attempt_no,
        status_update_value.lease_token
    );
    if(!locked_submission_exp){
        return std::unexpected(locked_submission_exp.error());
    }

    const auto persist_transition_exp = persist_submission_status_transition(
        transaction,
        submission_id,
        locked_submission_exp->status,
        status_update_value.to_status,
        status_update_value.reason_opt
    );
    if(!persist_transition_exp){
        return std::unexpected(persist_transition_exp.error());
    }

    return user_problem_summary_repository::apply_submission_status_transition(
        transaction,
        locked_submission_exp->user_id,
        locked_submission_exp->problem_id,
        locked_submission_exp->status,
        status_update_value.to_status
    );
}

std::expected<submission_response_dto::queued_response, repository_error>
submission_finalize_repository::rejudge_submission(
    pqxx::transaction_base& transaction,
    std::int64_t submission_id
){
    const auto locked_submission_exp = get_locked_submission_context(
        transaction,
        submission_id
    );
    if(!locked_submission_exp){
        return std::unexpected(locked_submission_exp.error());
    }

    if(
        locked_submission_exp->status == submission_status::queued ||
        locked_submission_exp->status == submission_status::judging
    ){
        return std::unexpected(repository_error::conflict);
    }

    if(locked_submission_exp->status == submission_status::accepted){
        const problem_dto::reference problem_reference_value{
            locked_submission_exp->problem_id
        };
        const auto decrease_accepted_count_exp =
            problem_statistics_repository::decrease_accepted_count(
                transaction,
                problem_reference_value
            );
        if(!decrease_accepted_count_exp){
            return std::unexpected(decrease_accepted_count_exp.error());
        }
    }

    const auto clear_submission_result_exp = clear_submission_result(
        transaction,
        submission_id
    );
    if(!clear_submission_result_exp){
        return std::unexpected(clear_submission_result_exp.error());
    }

    submission_internal_dto::status_update status_update_value;
    status_update_value.to_status = submission_status::queued;
    const auto persist_transition_exp = persist_submission_status_transition(
        transaction,
        submission_id,
        locked_submission_exp->status,
        status_update_value.to_status,
        status_update_value.reason_opt
    );
    if(!persist_transition_exp){
        return std::unexpected(persist_transition_exp.error());
    }

    const auto update_summary_exp =
        user_problem_summary_repository::apply_submission_status_transition(
            transaction,
            locked_submission_exp->user_id,
            locked_submission_exp->problem_id,
            locked_submission_exp->status,
            status_update_value.to_status
        );
    if(!update_summary_exp){
        return std::unexpected(update_summary_exp.error());
    }

    const auto enqueue_submission_exp = submission_queue_repository::enqueue_submission(
        transaction,
        submission_id,
        submission_queue_repository::REJUDGE_SUBMISSION_QUEUE_PRIORITY
    );
    if(!enqueue_submission_exp){
        return std::unexpected(enqueue_submission_exp.error());
    }

    return submission_response_dto::make_queued_response(
        submission_id,
        submission_status::queued,
        locked_submission_exp->problem_version
    );
}

std::expected<submission_internal_dto::finalize_result, repository_error>
submission_finalize_repository::finalize_submission(
    pqxx::transaction_base& transaction,
    const submission_internal_dto::finalize_request& finalize_request_value
){
    if(!submission_internal_dto::is_valid(finalize_request_value)){
        return std::unexpected(repository_error::invalid_reference);
    }

    const std::int64_t submission_id = finalize_request_value.submission_id;
    const auto locked_submission_exp = get_locked_leased_submission_context(
        transaction,
        submission_id,
        finalize_request_value.attempt_no,
        finalize_request_value.lease_token
    );
    if(!locked_submission_exp){
        return std::unexpected(locked_submission_exp.error());
    }

    const bool should_increase_accepted_count =
        finalize_request_value.to_status == submission_status::accepted &&
        locked_submission_exp->status != submission_status::accepted;
    transaction.exec(
        "UPDATE submissions "
        "SET "
        "status = $2::submission_status, "
        "score = $3, "
        "compile_output = $4, "
        "judge_output = $5, "
        "elapsed_ms = $6, "
        "max_rss_kb = $7, "
        "updated_at = NOW() "
        "WHERE submission_id = $1",
        pqxx::params{
            submission_id,
            to_string(finalize_request_value.to_status),
            finalize_request_value.score_opt,
            finalize_request_value.compile_output_opt,
            finalize_request_value.judge_output_opt,
            finalize_request_value.elapsed_ms_opt,
            finalize_request_value.max_rss_kb_opt
        }
    );

    transaction.exec(
        "INSERT INTO submission_status_history(submission_id, from_status, to_status, reason) "
        "VALUES($1, $2::submission_status, $3::submission_status, $4)",
        pqxx::params{
            submission_id,
            to_string(locked_submission_exp->status),
            to_string(finalize_request_value.to_status),
            finalize_request_value.reason_opt
        }
    );

    transaction.exec(
        "DELETE FROM submission_queue WHERE submission_id = $1",
        pqxx::params{submission_id}
    );

    const auto update_summary_exp =
        user_problem_summary_repository::apply_submission_status_transition(
            transaction,
            locked_submission_exp->user_id,
            locked_submission_exp->problem_id,
            locked_submission_exp->status,
            finalize_request_value.to_status
        );
    if(!update_summary_exp){
        return std::unexpected(update_summary_exp.error());
    }

    return submission_internal_dto::make_finalize_result(
        locked_submission_exp->problem_id,
        should_increase_accepted_count
    );
}
