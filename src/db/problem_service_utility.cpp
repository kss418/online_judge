#include "db/problem_service_utility.hpp"

#include <pqxx/pqxx>

namespace problem_service_utility{
std::expected<bool, error_code> exists_problem(
    pqxx::transaction_base& transaction,
    std::int64_t problem_id
){
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto exists_query_result = transaction.exec_params(
        "SELECT EXISTS("
        "SELECT 1 "
        "FROM problems "
        "WHERE problem_id = $1"
        ")",
        problem_id
    );

    if(exists_query_result.empty()){
        return std::unexpected(error_code::create(errno_error::unknown_error));
    }

    const bool is_exists = exists_query_result[0][0].as<bool>();
    return is_exists;
}

std::expected<void, error_code> increase_version(
    pqxx::transaction_base& transaction,
    std::int64_t problem_id
){
    const auto update_result = transaction.exec_params(
        "UPDATE problems "
        "SET version = version + 1 "
        "WHERE problem_id = $1",
        problem_id
    );

    if(update_result.affected_rows() == 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return {};
}
}
