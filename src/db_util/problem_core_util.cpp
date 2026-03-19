#include "db_util/problem_core_util.hpp"

#include <pqxx/pqxx>

std::expected<bool, error_code> problem_core_util::exists_problem(
    pqxx::transaction_base& transaction,
    std::int64_t problem_id
){
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto exists_query_result = transaction.exec(
        "SELECT EXISTS("
        "SELECT 1 "
        "FROM problems "
        "WHERE problem_id = $1"
        ")",
        pqxx::params{problem_id}
    );

    if(exists_query_result.empty()){
        return std::unexpected(error_code::create(errno_error::unknown_error));
    }

    return exists_query_result[0][0].as<bool>();
}

std::expected<std::int32_t, error_code> problem_core_util::get_version(
    pqxx::transaction_base& transaction,
    std::int64_t problem_id
){
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

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

std::expected<std::int64_t, error_code> problem_core_util::create_problem(
    pqxx::transaction_base& transaction
){
    const auto create_problem_result = transaction.exec(
        "INSERT INTO problems(version) "
        "VALUES($1) "
        "RETURNING problem_id",
        pqxx::params{1}
    );

    if(create_problem_result.empty()){
        return std::unexpected(error_code::create(errno_error::unknown_error));
    }

    return create_problem_result[0][0].as<std::int64_t>();
}

std::expected<problem_dto::limits, error_code> problem_core_util::get_limits(
    pqxx::transaction_base& transaction,
    std::int64_t problem_id
){
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

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

std::expected<void, error_code> problem_core_util::set_limits(
    pqxx::transaction_base& transaction,
    std::int64_t problem_id,
    const problem_dto::limits& limits_value
){
    if(problem_id <= 0 || limits_value.memory_mb <= 0 || limits_value.time_ms <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

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

    return {};
}

std::expected<void, error_code> problem_core_util::increase_version(
    pqxx::transaction_base& transaction,
    std::int64_t problem_id
){
    const auto update_result = transaction.exec(
        "UPDATE problems "
        "SET version = version + 1 "
        "WHERE problem_id = $1",
        pqxx::params{problem_id}
    );

    if(update_result.affected_rows() == 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return {};
}
