#include "db_util/submission_util.hpp"
#include "db_util/problem_statistics_util.hpp"

#include <pqxx/pqxx>

#include <string>

std::expected<submission_dto::history_list, error_code> submission_util::get_submission_history(
    pqxx::transaction_base& transaction,
    std::int64_t submission_id
){
    if(submission_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
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
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return submission_dto::make_history_list_from_result(submission_history_query);
}

std::expected<submission_dto::source_detail, error_code> submission_util::get_submission_source(
    pqxx::transaction_base& transaction,
    std::int64_t submission_id
){
    if(submission_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
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
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return submission_dto::make_source_detail_from_row(submission_source_query[0]);
}

std::expected<submission_dto::detail, error_code> submission_util::get_submission_detail(
    pqxx::transaction_base& transaction,
    std::int64_t submission_id
){
    if(submission_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
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
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return submission_dto::make_detail_from_row(submission_detail_result[0]);
}

std::expected<std::vector<submission_dto::summary>, error_code>
submission_util::get_wa_or_ac_submissions(
    pqxx::transaction_base& transaction,
    std::int64_t problem_id
){
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto submission_summary_query = transaction.exec(
        "SELECT "
        "submission_id, "
        "user_id, "
        "problem_id, "
        "language, "
        "status::text, "
        "score, "
        "elapsed_ms, "
        "max_rss_kb, "
        "created_at::text, "
        "updated_at::text "
        "FROM submissions "
        "WHERE "
        "problem_id = $1 AND "
        "("
        "status = $2::submission_status OR "
        "status = $3::submission_status"
        ") "
        "ORDER BY submission_id DESC",
        pqxx::params{
            problem_id,
            to_string(submission_status::wrong_answer),
            to_string(submission_status::accepted)
        }
    );

    return submission_dto::make_summary_list_from_result(submission_summary_query);
}

std::expected<submission_status, error_code> submission_util::get_submission_status(
    pqxx::transaction_base& transaction,
    std::int64_t submission_id
){
    if(submission_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto submission_status_result = transaction.exec(
        "SELECT status::text "
        "FROM submissions "
        "WHERE submission_id = $1",
        pqxx::params{submission_id}
    );
    if(submission_status_result.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const std::string submission_status_string = submission_status_result[0][0].as<std::string>();
    const auto submission_status_opt = parse_submission_status(submission_status_string);
    if(!submission_status_opt){
        return std::unexpected(error_code::create(errno_error::unknown_error));
    }

    return *submission_status_opt;
}

std::expected<submission_dto::created, error_code> submission_util::create_submission(
    pqxx::transaction_base& transaction,
    const submission_dto::create_request& create_request_value
){
    if(
        create_request_value.user_id <= 0 ||
        create_request_value.problem_id <= 0 ||
        create_request_value.source_value.language.empty() ||
        create_request_value.source_value.source_code.empty()
    ){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
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
        return std::unexpected(error_code::create(errno_error::unknown_error));
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

std::expected<void, error_code> submission_util::enqueue_submission(
    pqxx::transaction_base& transaction,
    std::int64_t submission_id,
    std::int16_t priority
){
    if(submission_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
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

std::expected<void, error_code> submission_util::update_submission_status(
    pqxx::transaction_base& transaction,
    const submission_dto::status_update& status_update_value
){
    const std::int64_t submission_id = status_update_value.submission_id;
    if(submission_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto current_status_result = transaction.exec(
        "SELECT status::text FROM submissions WHERE submission_id = $1 FOR UPDATE",
        pqxx::params{submission_id}
    );

    if(current_status_result.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const std::string from_status = current_status_result[0][0].as<std::string>();
    transaction.exec(
        "UPDATE submissions "
        "SET status = $2::submission_status, updated_at = NOW() "
        "WHERE submission_id = $1",
        pqxx::params{submission_id, to_string(status_update_value.to_status)}
    );

    transaction.exec(
        "INSERT INTO submission_status_history(submission_id, from_status, to_status, reason) "
        "VALUES($1, $2::submission_status, $3::submission_status, $4)",
        pqxx::params{
            submission_id,
            from_status,
            to_string(status_update_value.to_status),
            status_update_value.reason_opt
        }
    );

    return {};
}

std::expected<void, error_code> submission_util::clear_submission_result(
    pqxx::transaction_base& transaction,
    std::int64_t submission_id
){
    if(submission_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
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
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return {};
}

std::expected<void, error_code> submission_util::decrease_accepted_count_if_submission_accepted(
    pqxx::transaction_base& transaction,
    std::int64_t submission_id
){
    const auto submission_status_exp = get_submission_status(
        transaction,
        submission_id
    );
    if(!submission_status_exp){
        return std::unexpected(submission_status_exp.error());
    }

    if(*submission_status_exp != submission_status::accepted){
        return {};
    }

    const auto problem_result = transaction.exec(
        "SELECT problem_id "
        "FROM submissions "
        "WHERE submission_id = $1 AND status = $2::submission_status "
        "FOR UPDATE",
        pqxx::params{
            submission_id,
            to_string(submission_status::accepted)
        }
    );
    if(problem_result.empty()){
        return {};
    }

    const problem_dto::reference problem_reference_value{
        problem_result[0][0].as<std::int64_t>()
    };
    return problem_statistics_util::decrease_accepted_count(
        transaction,
        problem_reference_value
    );
}

std::expected<void, error_code> submission_util::rejudge_submission(
    pqxx::transaction_base& transaction,
    std::int64_t submission_id
){
    const auto submission_status_exp = get_submission_status(
        transaction,
        submission_id
    );
    if(!submission_status_exp){
        return std::unexpected(submission_status_exp.error());
    }

    if(
        *submission_status_exp == submission_status::queued ||
        *submission_status_exp == submission_status::judging
    ){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto decrease_accepted_count_exp =
        decrease_accepted_count_if_submission_accepted(
            transaction,
            submission_id
        );
    if(!decrease_accepted_count_exp){
        return std::unexpected(decrease_accepted_count_exp.error());
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
    const auto update_submission_status_exp = update_submission_status(
        transaction,
        status_update_value
    );
    if(!update_submission_status_exp){
        return std::unexpected(update_submission_status_exp.error());
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

std::expected<submission_dto::queued_submission, error_code> submission_util::lease_submission(
    pqxx::transaction_base& transaction,
    const submission_dto::lease_request& lease_request_value
){
    const std::chrono::seconds lease_duration = lease_request_value.lease_duration;
    if(lease_duration <= std::chrono::seconds::zero()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto lease_candidate_result = transaction.exec(
        "SELECT "
        "queue_table.submission_id, "
        "submission_table.problem_id, "
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
        return std::unexpected(error_code::create(errno_error::resource_temporarily_unavailable));
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

std::expected<submission_dto::finalize_result, error_code> submission_util::finalize_submission(
    pqxx::transaction_base& transaction,
    const submission_dto::finalize_request& finalize_request_value
){
    const std::int64_t submission_id = finalize_request_value.submission_id;
    if(submission_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto current_status_result = transaction.exec(
        "SELECT status::text, problem_id FROM submissions WHERE submission_id = $1 FOR UPDATE",
        pqxx::params{submission_id}
    );

    if(current_status_result.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const std::string from_status = current_status_result[0][0].as<std::string>();
    const std::int64_t problem_id = current_status_result[0][1].as<std::int64_t>();
    const bool should_increase_accepted_count =
        finalize_request_value.to_status == submission_status::accepted &&
        from_status != to_string(submission_status::accepted);
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
            from_status,
            to_string(finalize_request_value.to_status),
            finalize_request_value.reason_opt
        }
    );

    transaction.exec(
        "DELETE FROM submission_queue WHERE submission_id = $1",
        pqxx::params{submission_id}
    );

    return submission_dto::make_finalize_result(
        problem_id,
        should_increase_accepted_count
    );
}

std::expected<std::vector<submission_dto::summary>, error_code> submission_util::list_submissions(
    pqxx::transaction_base& transaction,
    const submission_dto::list_filter& filter_value
){
    if(
        (filter_value.top_submission_id_opt && *filter_value.top_submission_id_opt <= 0) ||
        (filter_value.user_id_opt && *filter_value.user_id_opt <= 0) ||
        (filter_value.problem_id_opt && *filter_value.problem_id_opt <= 0) ||
        (filter_value.limit_opt && *filter_value.limit_opt <= 0) ||
        (filter_value.status_opt && !parse_submission_status(*filter_value.status_opt))
    ){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const std::int32_t limit =
        filter_value.limit_opt.value_or(submission_dto::DEFAULT_LIST_LIMIT);

    std::string submission_list_query =
        "SELECT "
        "submission_id, "
        "user_id, "
        "problem_id, "
        "language, "
        "status::text, "
        "score, "
        "elapsed_ms, "
        "max_rss_kb, "
        "created_at::text, "
        "updated_at::text "
        "FROM submissions "
        "WHERE 1 = 1";
    pqxx::params query_params;
    int query_param_index = 1;

    if(filter_value.top_submission_id_opt){
        submission_list_query +=
            " AND submission_id <= $" + std::to_string(query_param_index++);
        query_params.append(*filter_value.top_submission_id_opt);
    }
    if(filter_value.user_id_opt){
        submission_list_query +=
            " AND user_id = $" + std::to_string(query_param_index++);
        query_params.append(*filter_value.user_id_opt);
    }
    if(filter_value.problem_id_opt){
        submission_list_query +=
            " AND problem_id = $" + std::to_string(query_param_index++);
        query_params.append(*filter_value.problem_id_opt);
    }
    if(filter_value.status_opt){
        submission_list_query +=
            " AND status = $" + std::to_string(query_param_index++) + "::submission_status";
        query_params.append(*filter_value.status_opt);
    }

    submission_list_query +=
        " ORDER BY submission_id DESC LIMIT $" + std::to_string(query_param_index++);
    query_params.append(limit);

    const auto submission_summary_query = transaction.exec(
        submission_list_query,
        query_params
    );

    return submission_dto::make_summary_list_from_result(submission_summary_query);
}
