#include "db_util/submission_util.hpp"

#include <pqxx/pqxx>

#include <string>

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

    submission_dto::detail detail_value;
    detail_value.submission_id = submission_detail_result[0][0].as<std::int64_t>();
    detail_value.user_id = submission_detail_result[0][1].as<std::int64_t>();
    detail_value.problem_id = submission_detail_result[0][2].as<std::int64_t>();
    detail_value.language = submission_detail_result[0][3].as<std::string>();
    detail_value.status = submission_detail_result[0][4].as<std::string>();
    if(!submission_detail_result[0][5].is_null()){
        detail_value.score_opt = submission_detail_result[0][5].as<std::int16_t>();
    }
    if(!submission_detail_result[0][6].is_null()){
        detail_value.compile_output_opt = submission_detail_result[0][6].as<std::string>();
    }
    if(!submission_detail_result[0][7].is_null()){
        detail_value.judge_output_opt = submission_detail_result[0][7].as<std::string>();
    }
    if(!submission_detail_result[0][8].is_null()){
        detail_value.elapsed_ms_opt = submission_detail_result[0][8].as<std::int64_t>();
    }
    if(!submission_detail_result[0][9].is_null()){
        detail_value.max_rss_kb_opt = submission_detail_result[0][9].as<std::int64_t>();
    }
    detail_value.created_at = submission_detail_result[0][10].as<std::string>();
    detail_value.updated_at = submission_detail_result[0][11].as<std::string>();
    return detail_value;
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

    transaction.exec(
        "INSERT INTO submission_queue(submission_id) VALUES($1)",
        pqxx::params{submission_id}
    );

    transaction.exec(
        "SELECT pg_notify($1, $2)",
        pqxx::params{SUBMISSION_QUEUE_CHANNEL, std::to_string(submission_id)}
    );

    submission_dto::created created_value;
    created_value.submission_id = submission_id;
    created_value.status = to_string(submission_status::queued);
    return created_value;
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

    submission_dto::queued_submission queued_submission_value;
    queued_submission_value.submission_id = lease_candidate_result[0][0].as<std::int64_t>();
    queued_submission_value.problem_id = lease_candidate_result[0][1].as<std::int64_t>();
    queued_submission_value.language = lease_candidate_result[0][2].as<std::string>();
    queued_submission_value.source_code = lease_candidate_result[0][3].as<std::string>();

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

    submission_dto::finalize_result finalize_result_value;
    finalize_result_value.problem_id = problem_id;
    finalize_result_value.should_increase_accepted_count = should_increase_accepted_count;
    return finalize_result_value;
}

std::expected<std::vector<submission_dto::summary>, error_code> submission_util::list_submissions(
    pqxx::transaction_base& transaction,
    const submission_dto::list_filter& filter_value
){
    if(
        (filter_value.top_submission_id_opt && *filter_value.top_submission_id_opt <= 0) ||
        (filter_value.user_id_opt && *filter_value.user_id_opt <= 0) ||
        (filter_value.problem_id_opt && *filter_value.problem_id_opt <= 0) ||
        (filter_value.status_opt && !parse_submission_status(*filter_value.status_opt))
    ){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

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

    submission_list_query += " ORDER BY submission_id DESC LIMIT 20";

    const auto submission_summary_query = transaction.exec(
        submission_list_query,
        query_params
    );

    std::vector<submission_dto::summary> summary_values;
    summary_values.reserve(submission_summary_query.size());
    for(const auto& submission_summary_row : submission_summary_query){
        submission_dto::summary summary_value;
        summary_value.submission_id = submission_summary_row[0].as<std::int64_t>();
        summary_value.user_id = submission_summary_row[1].as<std::int64_t>();
        summary_value.problem_id = submission_summary_row[2].as<std::int64_t>();
        summary_value.language = submission_summary_row[3].as<std::string>();
        summary_value.status = submission_summary_row[4].as<std::string>();
        if(!submission_summary_row[5].is_null()){
            summary_value.score_opt = submission_summary_row[5].as<std::int16_t>();
        }
        if(!submission_summary_row[6].is_null()){
            summary_value.elapsed_ms_opt = submission_summary_row[6].as<std::int64_t>();
        }
        if(!submission_summary_row[7].is_null()){
            summary_value.max_rss_kb_opt = submission_summary_row[7].as<std::int64_t>();
        }
        summary_value.created_at = submission_summary_row[8].as<std::string>();
        summary_value.updated_at = submission_summary_row[9].as<std::string>();
        summary_values.push_back(std::move(summary_value));
    }

    return summary_values;
}
