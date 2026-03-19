#include "db_service/problem_core_service.hpp"
#include "db_service/problem_statistics_service.hpp"
#include "db_util/problem_util.hpp"

#include <pqxx/pqxx>

#include <utility>

std::expected<bool, error_code> problem_core_service::exists_problem(
    db_connection& connection,
    std::int64_t problem_id
){
    if(!connection.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }

    try{
        pqxx::work transaction(connection.connection());
        const auto exists_exp = problem_util::exists_problem(transaction, problem_id);
        if(!exists_exp){
            return std::unexpected(exists_exp.error());
        }

        transaction.commit();
        return exists_exp.value();
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<std::int32_t, error_code> problem_core_service::get_version(
    db_connection& connection,
    std::int64_t problem_id
){
    if(!connection.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    try{
        pqxx::work transaction(connection.connection());
        const auto version_query_result = transaction.exec(
            "SELECT version "
            "FROM problems "
            "WHERE problem_id = $1",
            pqxx::params{problem_id}
        );

        if(version_query_result.empty()){
            return std::unexpected(error_code::create(errno_error::invalid_argument));
        }

        return version_query_result[0][0].as<std::int32_t>();
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<void, error_code> problem_core_service::increase_version(
    db_connection& connection,
    std::int64_t problem_id
){
    if(!connection.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }

    try{
        pqxx::work transaction(connection.connection());
        const auto version_exp = problem_util::increase_version(transaction, problem_id);
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

std::expected<std::int64_t, error_code> problem_core_service::create_problem(db_connection& connection){
    if(!connection.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }

    try{
        pqxx::work transaction(connection.connection());
        const auto create_problem_result = transaction.exec(
            "INSERT INTO problems(version) "
            "VALUES($1) "
            "RETURNING problem_id",
            pqxx::params{1}
        );

        if(create_problem_result.empty()){
            return std::unexpected(error_code::create(errno_error::unknown_error));
        }

        const std::int64_t problem_id = create_problem_result[0][0].as<std::int64_t>();
        const auto create_limits_result = transaction.exec(
            "INSERT INTO problem_limits(problem_id, memory_limit_mb, time_limit_ms, updated_at) "
            "VALUES($1, $2, $3, NOW())",
            pqxx::params{
                problem_id,
                problem_core_service::INITIAL_MEMORY_LIMIT_MB,
                problem_core_service::INITIAL_TIME_LIMIT_MS
            }
        );
        if(create_limits_result.affected_rows() == 0){
            return std::unexpected(error_code::create(errno_error::unknown_error));
        }

        const auto create_problem_statistics_exp = problem_statistics_service::create_problem_statistics(
            transaction,
            problem_id
        );
        if(!create_problem_statistics_exp){
            return std::unexpected(create_problem_statistics_exp.error());
        }

        const auto ensure_statement_exp = problem_util::ensure_statement_row(
            transaction,
            problem_id
        );
        if(!ensure_statement_exp){
            return std::unexpected(ensure_statement_exp.error());
        }

        transaction.commit();
        return problem_id;
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<problem_dto::limits, error_code> problem_core_service::get_limits(
    db_connection& connection,
    std::int64_t problem_id
){
    if(!connection.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    try{
        pqxx::work transaction(connection.connection());
        const auto limits_query_result = transaction.exec(
            "SELECT memory_limit_mb, time_limit_ms "
            "FROM problem_limits "
            "WHERE problem_id = $1",
            pqxx::params{problem_id}
        );

        if(limits_query_result.empty()){
            return std::unexpected(error_code::create(errno_error::invalid_argument));
        }

        problem_dto::limits limits_value;
        limits_value.memory_mb = limits_query_result[0][0].as<std::int32_t>();
        limits_value.time_ms = limits_query_result[0][1].as<std::int32_t>();
        return limits_value;
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<void, error_code> problem_core_service::set_limits(
    db_connection& connection,
    std::int64_t problem_id,
    const problem_dto::limits& limits_value
){
    if(!connection.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(problem_id <= 0 || limits_value.memory_mb <= 0 || limits_value.time_ms <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    try{
        pqxx::work transaction(connection.connection());
        transaction.exec(
            "INSERT INTO problem_limits(problem_id, memory_limit_mb, time_limit_ms, updated_at) "
            "VALUES($1, $2, $3, NOW()) "
            "ON CONFLICT(problem_id) DO UPDATE "
            "SET "
            "memory_limit_mb = EXCLUDED.memory_limit_mb, "
            "time_limit_ms = EXCLUDED.time_limit_ms, "
            "updated_at = NOW()",
            pqxx::params{
                problem_id,
                limits_value.memory_mb,
                limits_value.time_ms
            }
        );

        const auto version_exp = problem_util::increase_version(transaction, problem_id);
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
