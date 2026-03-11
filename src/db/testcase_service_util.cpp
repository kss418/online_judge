#include "db/testcase_service_util.hpp"

#include <pqxx/pqxx>

std::expected<std::int32_t, error_code> tc_service_util::increase_tc_count(
    pqxx::transaction_base& transaction,
    std::int64_t problem_id
){
    const auto increase_result = transaction.exec(
        "UPDATE problem_statements "
        "SET testcase_count = testcase_count + 1, updated_at = NOW() "
        "WHERE problem_id = $1 "
        "RETURNING testcase_count",
        pqxx::params{problem_id}
    );

    if(increase_result.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return increase_result[0][0].as<std::int32_t>();
}

std::expected<std::int32_t, error_code> tc_service_util::decrease_tc_count(
    pqxx::transaction_base& transaction,
    std::int64_t problem_id
){
    const auto decrease_result = transaction.exec(
        "UPDATE problem_statements "
        "SET testcase_count = testcase_count - 1, updated_at = NOW() "
        "WHERE problem_id = $1 AND testcase_count > 0 "
        "RETURNING testcase_count",
        pqxx::params{problem_id}
    );

    if(decrease_result.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return decrease_result[0][0].as<std::int32_t>();
}
