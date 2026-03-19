#include "db_util/testcase_util.hpp"

#include "db_util/problem_util.hpp"

#include <pqxx/pqxx>
#include <utility>

std::expected<std::int32_t, error_code> testcase_util::increase_tc_count(
    pqxx::transaction_base& transaction,
    std::int64_t problem_id
){
    const auto ensure_statement_exp = problem_util::ensure_statement_row(
        transaction,
        problem_id
    );
    if(!ensure_statement_exp){
        return std::unexpected(ensure_statement_exp.error());
    }

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

std::expected<std::int32_t, error_code> testcase_util::decrease_tc_count(
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

std::expected<std::vector<problem_dto::tc>, error_code> testcase_util::list_tcs(
    pqxx::transaction_base& transaction,
    std::int64_t problem_id
){
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto tcs_query_result = transaction.exec(
        "SELECT testcase_id, testcase_order, testcase_input, testcase_output "
        "FROM problem_testcases "
        "WHERE problem_id = $1 "
        "ORDER BY testcase_order ASC",
        pqxx::params{problem_id}
    );

    std::vector<problem_dto::tc> tc_values;
    tc_values.reserve(tcs_query_result.size());
    for(const auto& row : tcs_query_result){
        problem_dto::tc tc_value;
        tc_value.id = row[0].as<std::int64_t>();
        tc_value.order = row[1].as<std::int32_t>();
        tc_value.input = row[2].as<std::string>();
        tc_value.output = row[3].as<std::string>();
        tc_values.push_back(std::move(tc_value));
    }

    return tc_values;
}

std::expected<void, error_code> testcase_util::set_tc(
    pqxx::transaction_base& transaction,
    std::int64_t problem_id,
    const problem_dto::tc& tc_value
){
    if(problem_id <= 0 || tc_value.order <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto update_result = transaction.exec(
        "UPDATE problem_testcases "
        "SET "
        "testcase_input = $3, "
        "testcase_output = $4 "
        "WHERE problem_id = $1 AND testcase_order = $2",
        pqxx::params{
            problem_id,
            tc_value.order,
            tc_value.input,
            tc_value.output
        }
    );

    if(update_result.affected_rows() == 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto version_exp = problem_util::increase_version(transaction, problem_id);
    if(!version_exp){
        return std::unexpected(version_exp.error());
    }

    return {};
}

std::expected<void, error_code> testcase_util::delete_tc(
    pqxx::transaction_base& transaction,
    std::int64_t problem_id
){
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto delete_result = transaction.exec(
        "DELETE FROM problem_testcases "
        "WHERE "
        "problem_id = $1 AND "
        "testcase_order = ("
        "SELECT MAX(testcase_order) "
        "FROM problem_testcases "
        "WHERE problem_id = $1"
        ")",
        pqxx::params{problem_id}
    );

    if(delete_result.affected_rows() == 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto tc_count_exp = decrease_tc_count(transaction, problem_id);
    if(!tc_count_exp){
        return std::unexpected(tc_count_exp.error());
    }

    const auto version_exp = problem_util::increase_version(transaction, problem_id);
    if(!version_exp){
        return std::unexpected(version_exp.error());
    }

    return {};
}
