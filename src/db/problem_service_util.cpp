#include "db/problem_service_util.hpp"

#include <pqxx/pqxx>

std::expected<bool, error_code> problem_service_util::exists_problem(
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

std::expected<void, error_code> problem_service_util::increase_version(
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

std::expected<std::int32_t, error_code> problem_service_util::increase_sample_count(
    pqxx::transaction_base& transaction,
    std::int64_t problem_id
){
    const auto increase_result = transaction.exec(
        "UPDATE problem_statements "
        "SET sample_count = sample_count + 1, updated_at = NOW() "
        "WHERE problem_id = $1 "
        "RETURNING sample_count",
        pqxx::params{problem_id}
    );

    if(increase_result.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return increase_result[0][0].as<std::int32_t>();
}

std::expected<std::int32_t, error_code> problem_service_util::decrease_sample_count(
    pqxx::transaction_base& transaction,
    std::int64_t problem_id
){
    const auto decrease_result = transaction.exec(
        "UPDATE problem_statements "
        "SET sample_count = sample_count - 1, updated_at = NOW() "
        "WHERE problem_id = $1 AND sample_count > 0 "
        "RETURNING sample_count",
        pqxx::params{problem_id}
    );

    if(decrease_result.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }
    return decrease_result[0][0].as<std::int32_t>();
}
