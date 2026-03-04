#include "db/submission_service.hpp"

#include <pqxx/pqxx>

#include <chrono>
#include <utility>

submission_service::submission_service(db_connection connection) :
    db_service_base<submission_service>(std::move(connection)){}

std::expected<std::int64_t, error_code> submission_service::create_submission(
    std::int64_t user_id,
    std::int64_t problem_id,
    const std::string& language,
    const std::string& source_code
){
    if(!is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }

    try{
        pqxx::work transaction(connection());
        const auto result = transaction.exec_params(
            "INSERT INTO submissions(user_id, problem_id, language, source_code) "
            "VALUES($1, $2, $3, $4) "
            "RETURNING submission_id",
            user_id,
            problem_id,
            language,
            source_code
        );

        if(result.empty()){
            return std::unexpected(error_code::create(errno_error::unknown_error));
        }

        const std::int64_t submission_id = result[0][0].as<std::int64_t>();
        transaction.exec_params(
            "INSERT INTO submission_status_history(submission_id, from_status, to_status, reason) "
            "VALUES($1, NULL, $2::submission_status, NULL)",
            submission_id,
            to_string(submission_status::queued)
        );

        transaction.exec_params(
            "INSERT INTO submission_queue(submission_id) VALUES($1)",
            submission_id
        );
        transaction.exec_params(
            "SELECT pg_notify($1, $2)",
            submission_queue_channel_,
            std::to_string(submission_id)
        );

        transaction.commit();
        return submission_id;
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<void, error_code> submission_service::update_submission_status(
    std::int64_t submission_id,
    submission_status to_status,
    const std::optional<std::string>& reason
){
    if(!is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }

    try{
        pqxx::work transaction(connection());
        const auto current_status_result = transaction.exec_params(
            "SELECT status::text FROM submissions WHERE submission_id = $1 FOR UPDATE",
            submission_id
        );

        if(current_status_result.empty()){
            return std::unexpected(error_code::create(errno_error::invalid_argument));
        }

        const std::string from_status = current_status_result[0][0].as<std::string>();
        transaction.exec_params(
            "UPDATE submissions "
            "SET status = $2::submission_status, updated_at = NOW() "
            "WHERE submission_id = $1",
            submission_id,
            to_string(to_status)
        );

        transaction.exec_params(
            "INSERT INTO submission_status_history(submission_id, from_status, to_status, reason) "
            "VALUES($1, $2::submission_status, $3::submission_status, $4)",
            submission_id,
            from_status,
            to_string(to_status),
            reason
        );

        transaction.commit();
        return {};
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<void, error_code> submission_service::listen_submission_queue(){
    if(!is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }

    try{
        connection().listen(
            submission_queue_channel_,
            [](const pqxx::notification&){}
        );
        return {};
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<bool, error_code> submission_service::wait_submission_notification(
    std::chrono::milliseconds timeout
){
    if(!is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(timeout < std::chrono::milliseconds::zero()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    try{
        const auto timeout_seconds = std::chrono::duration_cast<std::chrono::seconds>(timeout);
        const auto remain = timeout - timeout_seconds;
        const auto timeout_microseconds =
            std::chrono::duration_cast<std::chrono::microseconds>(remain).count();

        const int notification_count = connection().await_notification(
            timeout_seconds.count(),
            static_cast<long>(timeout_microseconds)
        );
        return notification_count > 0;
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<queued_submission, error_code> submission_service::pop_submission(){
    if(!is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }

    try{
        pqxx::work transaction(connection());
        const auto pop_candidate_result = transaction.exec(
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

        if(pop_candidate_result.empty()){
            return std::unexpected(error_code::create(errno_error::resource_temporarily_unavailable));
        }

        queued_submission queued_submission_value;
        queued_submission_value.submission_id = pop_candidate_result[0][0].as<std::int64_t>();
        queued_submission_value.problem_id = pop_candidate_result[0][1].as<std::int64_t>();
        queued_submission_value.language = pop_candidate_result[0][2].as<std::string>();
        queued_submission_value.source_code = pop_candidate_result[0][3].as<std::string>();

        transaction.exec_params(
            "DELETE FROM submission_queue WHERE submission_id = $1",
            queued_submission_value.submission_id
        );

        transaction.commit();
        return queued_submission_value;
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<queued_submission, error_code> submission_service::lease_submission(
    std::chrono::seconds lease_duration
){
    if(!is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    
    if(lease_duration <= std::chrono::seconds::zero()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    try{
        pqxx::work transaction(connection());
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

        queued_submission queued_submission_value;
        queued_submission_value.submission_id = lease_candidate_result[0][0].as<std::int64_t>();
        queued_submission_value.problem_id = lease_candidate_result[0][1].as<std::int64_t>();
        queued_submission_value.language = lease_candidate_result[0][2].as<std::string>();
        queued_submission_value.source_code = lease_candidate_result[0][3].as<std::string>();

        const std::int64_t lease_seconds = lease_duration.count();
        transaction.exec_params(
            "UPDATE submission_queue "
            "SET "
            "leased_until = NOW() + ($2 * INTERVAL '1 second'), "
            "attempt_count = attempt_count + 1 "
            "WHERE submission_id = $1",
            queued_submission_value.submission_id,
            lease_seconds
        );

        transaction.commit();
        return queued_submission_value;
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<void, error_code> submission_service::finalize_submission(
    std::int64_t submission_id,
    submission_status to_status,
    std::optional<std::int16_t> score,
    std::optional<std::string> compile_output,
    std::optional<std::string> judge_output,
    std::optional<std::string> reason
){
    if(!is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(submission_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    try{
        pqxx::work transaction(connection());
        const auto current_status_result = transaction.exec_params(
            "SELECT status::text FROM submissions WHERE submission_id = $1 FOR UPDATE",
            submission_id
        );

        if(current_status_result.empty()){
            return std::unexpected(error_code::create(errno_error::invalid_argument));
        }

        const std::string from_status = current_status_result[0][0].as<std::string>();
        transaction.exec_params(
            "UPDATE submissions "
            "SET "
            "status = $2::submission_status, "
            "score = $3, "
            "compile_output = $4, "
            "judge_output = $5, "
            "updated_at = NOW() "
            "WHERE submission_id = $1",
            submission_id,
            to_string(to_status),
            score,
            compile_output,
            judge_output
        );

        transaction.exec_params(
            "INSERT INTO submission_status_history(submission_id, from_status, to_status, reason) "
            "VALUES($1, $2::submission_status, $3::submission_status, $4)",
            submission_id,
            from_status,
            to_string(to_status),
            reason
        );

        transaction.exec_params(
            "DELETE FROM submission_queue WHERE submission_id = $1",
            submission_id
        );

        transaction.commit();
        return {};
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
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
