#include "db_util/submission_util.hpp"
#include "db_service/problem_statistics_service.hpp"

#include <pqxx/pqxx>

#include <string>

std::expected<std::int64_t, error_code> submission_util::create_submission(
    pqxx::transaction_base& transaction,
    std::int64_t user_id,
    std::int64_t problem_id,
    const submission_dto::source& source_value
){
    if(
        user_id <= 0 ||
        problem_id <= 0 ||
        source_value.language.empty() ||
        source_value.source_code.empty()
    ){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto create_submission_result = transaction.exec(
        "INSERT INTO submissions(user_id, problem_id, language, source_code) "
        "VALUES($1, $2, $3, $4) "
        "RETURNING submission_id",
        pqxx::params{
            user_id,
            problem_id,
            source_value.language,
            source_value.source_code
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

    const auto increase_submission_count_exp = problem_statistics_service::increase_submission_count(
        transaction,
        problem_id
    );
    if(!increase_submission_count_exp){
        return std::unexpected(increase_submission_count_exp.error());
    }

    transaction.exec(
        "SELECT pg_notify($1, $2)",
        pqxx::params{SUBMISSION_QUEUE_CHANNEL, std::to_string(submission_id)}
    );

    return submission_id;
}

std::expected<void, error_code> submission_util::update_submission_status(
    pqxx::transaction_base& transaction,
    std::int64_t submission_id,
    submission_status to_status,
    const std::optional<std::string>& reason_opt
){
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
        pqxx::params{submission_id, to_string(to_status)}
    );

    transaction.exec(
        "INSERT INTO submission_status_history(submission_id, from_status, to_status, reason) "
        "VALUES($1, $2::submission_status, $3::submission_status, $4)",
        pqxx::params{submission_id, from_status, to_string(to_status), reason_opt}
    );

    return {};
}

std::expected<submission_dto::queued_submission, error_code> submission_util::lease_submission(
    pqxx::transaction_base& transaction,
    std::chrono::seconds lease_duration
){
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

std::expected<void, error_code> submission_util::finalize_submission(
    pqxx::transaction_base& transaction,
    std::int64_t submission_id,
    submission_status to_status,
    std::optional<std::int16_t> score_opt,
    std::optional<std::string> compile_output_opt,
    std::optional<std::string> judge_output_opt,
    std::optional<std::string> reason_opt
){
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
    transaction.exec(
        "UPDATE submissions "
        "SET "
        "status = $2::submission_status, "
        "score = $3, "
        "compile_output = $4, "
        "judge_output = $5, "
        "updated_at = NOW() "
        "WHERE submission_id = $1",
        pqxx::params{
            submission_id,
            to_string(to_status),
            score_opt,
            compile_output_opt,
            judge_output_opt
        }
    );

    transaction.exec(
        "INSERT INTO submission_status_history(submission_id, from_status, to_status, reason) "
        "VALUES($1, $2::submission_status, $3::submission_status, $4)",
        pqxx::params{submission_id, from_status, to_string(to_status), reason_opt}
    );

    if(
        to_status == submission_status::accepted &&
        from_status != to_string(submission_status::accepted)
    ){
        const auto increase_accepted_count_exp = problem_statistics_service::increase_accepted_count(
            transaction,
            problem_id
        );
        if(!increase_accepted_count_exp){
            return std::unexpected(increase_accepted_count_exp.error());
        }
    }

    transaction.exec(
        "DELETE FROM submission_queue WHERE submission_id = $1",
        pqxx::params{submission_id}
    );

    return {};
}

std::string to_string(submission_status status){
    switch(status){
        case submission_status::queued:
            return "queued";
        case submission_status::judging:
            return "judging";
        case submission_status::accepted:
            return "accepted";
        case submission_status::wrong_answer:
            return "wrong_answer";
        case submission_status::time_limit_exceeded:
            return "time_limit_exceeded";
        case submission_status::memory_limit_exceeded:
            return "memory_limit_exceeded";
        case submission_status::runtime_error:
            return "runtime_error";
        case submission_status::compile_error:
            return "compile_error";
        case submission_status::output_exceeded:
            return "output_exceeded";
    }

    return "queued";
}

std::optional<submission_status> parse_submission_status(std::string_view status){
    if(status == "queued"){
        return submission_status::queued;
    }
    if(status == "judging"){
        return submission_status::judging;
    }
    if(status == "accepted"){
        return submission_status::accepted;
    }
    if(status == "wrong_answer"){
        return submission_status::wrong_answer;
    }
    if(status == "time_limit_exceeded"){
        return submission_status::time_limit_exceeded;
    }
    if(status == "memory_limit_exceeded"){
        return submission_status::memory_limit_exceeded;
    }
    if(status == "runtime_error"){
        return submission_status::runtime_error;
    }
    if(status == "compile_error"){
        return submission_status::compile_error;
    }
    if(status == "output_exceeded"){
        return submission_status::output_exceeded;
    }

    return std::nullopt;
}
