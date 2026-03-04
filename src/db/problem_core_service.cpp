#include "db/problem_core_service.hpp"
#include "db/problem_service_utility.hpp"

#include <pqxx/pqxx>

#include <utility>

problem_core_service::problem_core_service(db_connection connection) :
    db_service_base<problem_core_service>(std::move(connection)){}

std::expected<std::int64_t, error_code> problem_core_service::create_problem(){
    if(!is_connected()){
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

std::expected<limits, error_code> problem_core_service::get_limits(std::int64_t problem_id){
    if(!is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    try{
        pqxx::work transaction(connection());
        const auto limits_query_result = transaction.exec_params(
            "SELECT memory_limit_mb, time_limit_ms "
            "FROM problem_limits "
            "WHERE problem_id = $1",
            problem_id
        );

        if(limits_query_result.empty()){
            return std::unexpected(error_code::create(errno_error::invalid_argument));
        }

        limits limits_value;
        limits_value.memory_limit_mb = limits_query_result[0][0].as<std::int32_t>();
        limits_value.time_limit_ms = limits_query_result[0][1].as<std::int32_t>();
        return limits_value;
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<void, error_code> problem_core_service::set_limits(
    std::int64_t problem_id,
    std::int32_t memory_limit_mb,
    std::int32_t time_limit_ms
){
    if(!is_connected()){
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

        const auto version_exp = problem_service_utility::increase_version(transaction, problem_id);
        if(!version_exp){
            return std::unexpected(version_exp.error());
        }

        transaction.commit();
        return {};
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}
