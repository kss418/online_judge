#include "db/submission_service.hpp"

#include <pqxx/pqxx>

#include <utility>

std::expected<submission_service, error_code> submission_service::create(){
    auto connection_exp = db_connection::create();
    if(!connection_exp){
        return std::unexpected(connection_exp.error());
    }

    return submission_service(std::move(*connection_exp));
}

submission_service::submission_service(db_connection connection) :
    db_connection_(std::move(connection)){}

pqxx::connection& submission_service::connection(){
    return db_connection_.connection();
}

const pqxx::connection& submission_service::connection() const{
    return db_connection_.connection();
}

std::expected<std::int64_t, error_code> submission_service::create_submission(const submission_create_request& request){
    if(!db_connection_.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }

    try{
        pqxx::work transaction(connection());
        const auto result = transaction.exec_params(
            "INSERT INTO submissions(user_id, problem_id, language, source_code) "
            "VALUES($1, $2, $3, $4) "
            "RETURNING submission_id",
            request.user_id,
            request.problem_id,
            request.language,
            request.source_code
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
    if(!db_connection_.is_connected()){
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
