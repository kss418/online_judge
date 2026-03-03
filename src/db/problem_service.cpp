#include "db/problem_service.hpp"

#include <pqxx/pqxx>

#include <utility>

std::expected<problem_service, error_code> problem_service::create(db_connection db_connection){
    if(!db_connection.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }

    return problem_service(std::move(db_connection));
}

problem_service::problem_service(db_connection connection) :
    db_connection_(std::move(connection)){}

pqxx::connection& problem_service::connection(){
    return db_connection_.connection();
}

const pqxx::connection& problem_service::connection() const{
    return db_connection_.connection();
}

std::expected<std::int64_t, error_code> problem_service::create_problem(){
    if(!db_connection_.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }

    try{
        pqxx::work transaction(connection());
        const auto create_problem_result = transaction.exec_params(
            "INSERT INTO problems(version) "
            "VALUES($1) "
            "RETURNING problem_id",
            1
        );

        if(create_problem_result.empty()){
            return std::unexpected(error_code::create(errno_error::unknown_error));
        }

        const std::int64_t problem_id = create_problem_result[0][0].as<std::int64_t>();

        transaction.commit();
        return problem_id;
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<void, error_code> problem_service::increase_problem_version(std::int64_t problem_id){
    if(!db_connection_.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    try{
        pqxx::work transaction(connection());
        const auto update_result = transaction.exec_params(
            "UPDATE problems "
            "SET version = version + 1 "
            "WHERE problem_id = $1",
            problem_id
        );

        if(update_result.affected_rows() == 0){
            return std::unexpected(error_code::create(errno_error::invalid_argument));
        }

        transaction.commit();
        return {};
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<void, error_code> problem_service::set_problem_limits(
    std::int64_t problem_id,
    std::int32_t memory_limit_mb,
    std::int32_t time_limit_ms
){
    if(!db_connection_.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(problem_id <= 0 || memory_limit_mb <= 0 || time_limit_ms <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    try{
        pqxx::work transaction(connection());
        transaction.exec_params(
            "INSERT INTO problem_limits(problem_id, memory_limit_mb, time_limit_ms, updated_at) "
            "VALUES($1, $2, $3, NOW()) "
            "ON CONFLICT(problem_id) DO UPDATE "
            "SET "
            "memory_limit_mb = EXCLUDED.memory_limit_mb, "
            "time_limit_ms = EXCLUDED.time_limit_ms, "
            "updated_at = NOW()",
            problem_id,
            memory_limit_mb,
            time_limit_ms
        );

        transaction.commit();
        return {};
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<void, error_code> problem_service::set_problem_statement(
    std::int64_t problem_id,
    const std::string& description,
    const std::string& input_format,
    const std::string& output_format,
    const std::string& note
){
    if(!db_connection_.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    try{
        pqxx::work transaction(connection());
        transaction.exec_params(
            "INSERT INTO problem_statements("
            "problem_id, description, input_format, output_format, note, created_at, updated_at"
            ") VALUES($1, $2, $3, $4, $5, NOW(), NOW()) "
            "ON CONFLICT(problem_id) DO UPDATE "
            "SET "
            "description = EXCLUDED.description, "
            "input_format = EXCLUDED.input_format, "
            "output_format = EXCLUDED.output_format, "
            "note = EXCLUDED.note, "
            "updated_at = NOW()",
            problem_id,
            description,
            input_format,
            output_format,
            note
        );

        transaction.commit();
        return {};
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}
