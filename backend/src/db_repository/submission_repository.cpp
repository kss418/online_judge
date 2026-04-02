#include "db_repository/submission_repository.hpp"
#include "db_repository/db_repository.hpp"
#include "db_repository/problem_statistics_repository.hpp"
#include "query_builder/submission_query_builder.hpp"
#include "db_repository/user_problem_summary_repository.hpp"

#include <pqxx/pqxx>

#include <utility>

namespace{
    std::int32_t resolve_submission_list_limit(
        const submission_dto::list_filter& filter_value
    ){
        return filter_value.limit_opt.value_or(submission_dto::DEFAULT_LIST_LIMIT);
    }
}

std::expected<submission_repository::locked_submission_context, repository_error>
submission_repository::get_locked_submission_context(
    pqxx::transaction_base& transaction,
    std::int64_t submission_id
){
    if(submission_id <= 0){
        return std::unexpected(db_repository::invalid_reference_error());
    }

    const auto submission_result = transaction.exec(
        "SELECT user_id, problem_id, status::text "
        "FROM submissions "
        "WHERE submission_id = $1 "
        "FOR UPDATE",
        pqxx::params{submission_id}
    );
    if(submission_result.empty()){
        return std::unexpected(db_repository::not_found_error());
    }

    const auto submission_status_exp = submission_dto::make_submission_status_from_row(
        submission_result[0],
        2
    );
    if(!submission_status_exp){
        return std::unexpected(db_repository::map_error(submission_status_exp.error()));
    }

    locked_submission_context context_value;
    context_value.user_id = submission_result[0][0].as<std::int64_t>();
    context_value.problem_id = submission_result[0][1].as<std::int64_t>();
    context_value.status = *submission_status_exp;
    return context_value;
}

std::expected<void, repository_error> submission_repository::persist_submission_status_transition(
    pqxx::transaction_base& transaction,
    std::int64_t submission_id,
    submission_status from_status,
    submission_status to_status,
    const std::optional<std::string>& reason_opt
){
    if(submission_id <= 0){
        return std::unexpected(db_repository::invalid_reference_error());
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

std::expected<submission_dto::history_list, repository_error> submission_repository::get_submission_history(
    pqxx::transaction_base& transaction,
    std::int64_t submission_id
){
    if(submission_id <= 0){
        return std::unexpected(db_repository::invalid_reference_error());
    }

    const auto submission_history_query = transaction.exec(
        "SELECT "
        "history_id, "
        "from_status::text, "
        "to_status::text, "
        "reason, "
        "created_at::text "
        "FROM submission_status_history "
        "WHERE submission_id = $1 "
        "ORDER BY history_id ASC",
        pqxx::params{submission_id}
    );
    if(submission_history_query.empty()){
        return std::unexpected(db_repository::not_found_error());
    }

    return submission_dto::make_history_list_from_result(submission_history_query);
}

std::expected<submission_dto::source_detail, repository_error> submission_repository::get_submission_source(
    pqxx::transaction_base& transaction,
    std::int64_t submission_id
){
    if(submission_id <= 0){
        return std::unexpected(db_repository::invalid_reference_error());
    }

    const auto submission_source_query = transaction.exec(
        "SELECT "
        "submission_id, "
        "user_id, "
        "problem_id, "
        "language, "
        "source_code, "
        "compile_output, "
        "judge_output "
        "FROM submissions "
        "WHERE submission_id = $1",
        pqxx::params{submission_id}
    );
    if(submission_source_query.empty()){
        return std::unexpected(db_repository::not_found_error());
    }

    return submission_dto::make_source_detail_from_row(submission_source_query[0]);
}

std::expected<submission_dto::detail, repository_error> submission_repository::get_submission_detail(
    pqxx::transaction_base& transaction,
    std::int64_t submission_id
){
    if(submission_id <= 0){
        return std::unexpected(db_repository::invalid_reference_error());
    }

    const auto submission_detail_result = transaction.exec(
        "SELECT "
        "submission_id, "
        "user_id, "
        "problem_id, "
        "language, "
        "status::text, "
        "score, "
        "compile_output, "
        "judge_output, "
        "elapsed_ms, "
        "max_rss_kb, "
        "created_at::text, "
        "updated_at::text "
        "FROM submissions "
        "WHERE submission_id = $1",
        pqxx::params{submission_id}
    );
    if(submission_detail_result.empty()){
        return std::unexpected(db_repository::not_found_error());
    }

    return submission_dto::make_detail_from_row(submission_detail_result[0]);
}

std::expected<std::vector<submission_dto::status_snapshot>, repository_error>
submission_repository::get_submission_status_snapshots(
    pqxx::transaction_base& transaction,
    const std::vector<std::int64_t>& submission_ids
){
    if(submission_ids.empty()){
        return std::vector<submission_dto::status_snapshot>{};
    }

    const auto query_exp = submission_query_builder::build_status_snapshot_query(
        submission_ids
    );
    if(!query_exp){
        return std::unexpected(db_repository::map_error(query_exp.error()));
    }

    const auto submission_status_result = transaction.exec(
        query_exp->sql,
        std::move(query_exp->params)
    );
    return submission_dto::make_status_snapshot_list_from_result(submission_status_result);
}

std::expected<std::vector<submission_dto::summary>, repository_error>
submission_repository::get_wa_or_ac_submissions(
    pqxx::transaction_base& transaction,
    std::int64_t problem_id
){
    if(problem_id <= 0){
        return std::unexpected(db_repository::invalid_reference_error());
    }

    const auto submission_summary_query = transaction.exec(
        "SELECT "
        "submission_table.submission_id, "
        "submission_table.user_id, "
        "COALESCE(user_table.user_login_id, ''), "
        "submission_table.problem_id, "
        "problem_table.title, "
        "submission_table.language, "
        "submission_table.status::text, "
        "submission_table.score, "
        "submission_table.elapsed_ms, "
        "submission_table.max_rss_kb, "
        "NULL::text, "
        "submission_table.created_at::text, "
        "submission_table.updated_at::text "
        "FROM submissions submission_table "
        "JOIN problems problem_table "
        "ON problem_table.problem_id = submission_table.problem_id "
        "JOIN users user_table "
        "ON user_table.user_id = submission_table.user_id "
        "WHERE "
        "submission_table.problem_id = $1 AND "
        "("
        "submission_table.status = $2::submission_status OR "
        "submission_table.status = $3::submission_status"
        ") "
        "ORDER BY submission_table.submission_id DESC",
        pqxx::params{
            problem_id,
            to_string(submission_status::wrong_answer),
            to_string(submission_status::accepted)
        }
    );

    return submission_dto::make_summary_list_from_result(submission_summary_query);
}

std::expected<submission_status, repository_error> submission_repository::get_submission_status(
    pqxx::transaction_base& transaction,
    std::int64_t submission_id
){
    if(submission_id <= 0){
        return std::unexpected(db_repository::invalid_reference_error());
    }

    const auto submission_status_result = transaction.exec(
        "SELECT status::text "
        "FROM submissions "
        "WHERE submission_id = $1",
        pqxx::params{submission_id}
    );
    if(submission_status_result.empty()){
        return std::unexpected(db_repository::not_found_error());
    }

    return db_repository::map_error(submission_dto::make_submission_status_from_row(
        submission_status_result[0],
        0
    ));
}

std::expected<submission_dto::created, repository_error> submission_repository::create_submission(
    pqxx::transaction_base& transaction,
    const submission_dto::create_request& create_request_value
){
    if(!submission_dto::is_valid(create_request_value)){
        return std::unexpected(db_repository::invalid_input_error());
    }

    const auto create_submission_result = transaction.exec(
        "INSERT INTO submissions(user_id, problem_id, language, source_code) "
        "VALUES($1, $2, $3, $4) "
        "RETURNING submission_id",
        pqxx::params{
            create_request_value.user_id,
            create_request_value.problem_id,
            create_request_value.source_value.language,
            create_request_value.source_value.source_code
        }
    );

    if(create_submission_result.empty()){
        return std::unexpected(db_repository::internal_error());
    }

    const std::int64_t submission_id = create_submission_result[0][0].as<std::int64_t>();
    transaction.exec(
        "INSERT INTO submission_status_history(submission_id, from_status, to_status, reason) "
        "VALUES($1, NULL, $2::submission_status, NULL)",
        pqxx::params{submission_id, to_string(submission_status::queued)}
    );

    return submission_dto::make_created(
        submission_id,
        submission_status::queued
    );
}

std::expected<void, repository_error> submission_repository::enqueue_submission(
    pqxx::transaction_base& transaction,
    std::int64_t submission_id,
    std::int16_t priority
){
    if(submission_id <= 0){
        return std::unexpected(db_repository::invalid_reference_error());
    }

    transaction.exec(
        "INSERT INTO submission_queue(submission_id, priority) VALUES($1, $2)",
        pqxx::params{submission_id, priority}
    );

    transaction.exec(
        "SELECT pg_notify($1, $2)",
        pqxx::params{SUBMISSION_QUEUE_CHANNEL, std::to_string(submission_id)}
    );

    return {};
}

std::expected<void, repository_error> submission_repository::update_submission_status(
    pqxx::transaction_base& transaction,
    const submission_dto::status_update& status_update_value
){
    if(!submission_dto::is_valid(status_update_value)){
        return std::unexpected(db_repository::invalid_reference_error());
    }

    const std::int64_t submission_id = status_update_value.submission_id;
    const auto locked_submission_exp = get_locked_submission_context(
        transaction,
        submission_id
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

std::expected<void, repository_error> submission_repository::clear_submission_result(
    pqxx::transaction_base& transaction,
    std::int64_t submission_id
){
    if(submission_id <= 0){
        return std::unexpected(db_repository::invalid_reference_error());
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
        return std::unexpected(db_repository::not_found_error());
    }

    return {};
}

std::expected<void, repository_error> submission_repository::rejudge_submission(
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
        return std::unexpected(db_repository::conflict_error());
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

    const submission_dto::status_update status_update_value =
        submission_dto::make_status_update(
            submission_id,
            submission_status::queued
        );
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

    const auto enqueue_submission_exp = enqueue_submission(
        transaction,
        submission_id,
        REJUDGE_SUBMISSION_QUEUE_PRIORITY
    );
    if(!enqueue_submission_exp){
        return std::unexpected(enqueue_submission_exp.error());
    }

    return {};
}

std::expected<submission_dto::queued_submission, repository_error> submission_repository::lease_submission(
    pqxx::transaction_base& transaction,
    const submission_dto::lease_request& lease_request_value
){
    const std::chrono::seconds lease_duration = lease_request_value.lease_duration;
    if(!submission_dto::is_valid(lease_request_value)){
        return std::unexpected(db_repository::invalid_input_error());
    }

    const auto lease_candidate_result = transaction.exec(
        "SELECT "
        "queue_table.submission_id, "
        "submission_table.problem_id, "
        "GREATEST(0::bigint, FLOOR(EXTRACT(EPOCH FROM (NOW() - queue_table.available_at)) * 1000))::bigint, "
        "submission_table.language, "
        "submission_table.source_code "
        "FROM submission_queue queue_table "
        "JOIN submissions submission_table "
        "ON submission_table.submission_id = queue_table.submission_id "
        "WHERE "
        "queue_table.available_at <= NOW() AND "
        "(queue_table.leased_until IS NULL OR queue_table.leased_until <= NOW()) "
        "ORDER BY queue_table.priority DESC, queue_table.created_at ASC "
        "FOR UPDATE SKIP LOCKED "
        "LIMIT 1"
    );

    if(lease_candidate_result.empty()){
        return std::unexpected(db_repository::not_found_error());
    }

    submission_dto::queued_submission queued_submission_value =
        submission_dto::make_queued_submission_from_row(
            lease_candidate_result[0]
        );

    transaction.exec(
        "UPDATE submission_queue "
        "SET "
        "leased_until = NOW() + ($2 * INTERVAL '1 second'), "
        "attempt_count = attempt_count + 1 "
        "WHERE submission_id = $1",
        pqxx::params{queued_submission_value.submission_id, lease_duration.count()}
    );

    return queued_submission_value;
}

std::expected<void, repository_error> submission_repository::release_submission_lease(
    pqxx::transaction_base& transaction,
    std::int64_t submission_id
){
    if(submission_id <= 0){
        return std::unexpected(db_repository::invalid_reference_error());
    }

    const auto update_result = transaction.exec(
        "UPDATE submission_queue "
        "SET "
        "available_at = NOW(), "
        "leased_until = NOW() "
        "WHERE submission_id = $1",
        pqxx::params{submission_id}
    );
    if(update_result.affected_rows() == 0){
        return std::unexpected(db_repository::not_found_error());
    }

    return {};
}

std::expected<submission_dto::finalize_result, repository_error> submission_repository::finalize_submission(
    pqxx::transaction_base& transaction,
    const submission_dto::finalize_request& finalize_request_value
){
    if(!submission_dto::is_valid(finalize_request_value)){
        return std::unexpected(db_repository::invalid_reference_error());
    }

    const std::int64_t submission_id = finalize_request_value.submission_id;
    const auto locked_submission_exp = get_locked_submission_context(
        transaction,
        submission_id
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

    return submission_dto::make_finalize_result(
        locked_submission_exp->problem_id,
        should_increase_accepted_count
    );
}

std::expected<submission_dto::summary_page, repository_error> submission_repository::list_submissions(
    pqxx::transaction_base& transaction,
    const submission_dto::list_filter& filter_value,
    std::optional<std::int64_t> viewer_user_id_opt
){
    auto query_exp = submission_query_builder::submission_list_query_builder{
        filter_value,
        viewer_user_id_opt
    }.build_list_query();
    if(!query_exp){
        return std::unexpected(db_repository::map_error(query_exp.error()));
    }

    const auto submission_summary_query = transaction.exec(
        query_exp->sql,
        std::move(query_exp->params)
    );

    auto summary_values =
        submission_dto::make_summary_list_from_result(submission_summary_query);
    submission_dto::summary_page summary_page_value;
    summary_page_value.has_more =
        summary_values.size() >
        static_cast<std::size_t>(resolve_submission_list_limit(filter_value));
    if(summary_page_value.has_more){
        summary_values.resize(
            static_cast<std::size_t>(resolve_submission_list_limit(filter_value))
        );
        if(!summary_values.empty()){
            summary_page_value.next_before_submission_id_opt =
                summary_values.back().submission_id;
        }
    }
    summary_page_value.submissions = std::move(summary_values);
    return summary_page_value;
}
