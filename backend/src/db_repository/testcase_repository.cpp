#include "db_repository/testcase_repository.hpp"

#include <pqxx/pqxx>
#include <utility>

std::expected<problem_dto::testcase, error_code> testcase_repository::create_testcase(
    pqxx::transaction_base& transaction,
    const problem_dto::testcase_ref& testcase_reference_value,
    const problem_dto::testcase& testcase_value
){
    const std::int64_t problem_id = testcase_reference_value.problem_id;
    const std::int32_t testcase_order = testcase_reference_value.testcase_order;
    if(problem_id <= 0 || testcase_order <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }
    const auto create_testcase_result = transaction.exec(
        "INSERT INTO problem_testcases(problem_id, testcase_order, testcase_input, testcase_output) "
        "VALUES($1, $2, $3, $4) "
        "RETURNING testcase_id",
        pqxx::params{
            problem_id,
            testcase_order,
            testcase_value.input,
            testcase_value.output
        }
    );

    if(create_testcase_result.empty()){
        return std::unexpected(error_code::create(errno_error::unknown_error));
    }

    problem_dto::testcase created_testcase_value = testcase_value;
    created_testcase_value.id = create_testcase_result[0][0].as<std::int64_t>();
    created_testcase_value.order = testcase_order;
    return created_testcase_value;
}

std::expected<problem_dto::testcase, error_code> testcase_repository::get_testcase(
    pqxx::transaction_base& transaction,
    const problem_dto::testcase_ref& testcase_reference_value
){
    const std::int64_t problem_id = testcase_reference_value.problem_id;
    const std::int32_t testcase_order = testcase_reference_value.testcase_order;
    if(problem_id <= 0 || testcase_order <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto testcase_query_result = transaction.exec(
        "SELECT testcase_id, testcase_order, testcase_input, testcase_output "
        "FROM problem_testcases "
        "WHERE problem_id = $1 AND testcase_order = $2",
        pqxx::params{problem_id, testcase_order}
    );

    if(testcase_query_result.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    problem_dto::testcase testcase_value;
    testcase_value.id = testcase_query_result[0][0].as<std::int64_t>();
    testcase_value.order = testcase_query_result[0][1].as<std::int32_t>();
    testcase_value.input = testcase_query_result[0][2].as<std::string>();
    testcase_value.output = testcase_query_result[0][3].as<std::string>();
    return testcase_value;
}

std::expected<problem_dto::testcase_count, error_code> testcase_repository::get_testcase_count(
    pqxx::transaction_base& transaction,
    const problem_dto::reference& problem_reference_value
){
    const std::int64_t problem_id = problem_reference_value.problem_id;
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto testcase_count_query_result = transaction.exec(
        "SELECT COUNT(*) "
        "FROM problem_testcases "
        "WHERE problem_id = $1",
        pqxx::params{problem_id}
    );

    if(testcase_count_query_result.empty()){
        return std::unexpected(error_code::create(errno_error::unknown_error));
    }

    problem_dto::testcase_count testcase_count_value;
    testcase_count_value.testcase_count = testcase_count_query_result[0][0].as<std::int32_t>();
    return testcase_count_value;
}

std::expected<problem_dto::testcase_count, error_code> testcase_repository::increase_testcase_count(
    pqxx::transaction_base& transaction,
    const problem_dto::reference& problem_reference_value
){
    const std::int64_t problem_id = problem_reference_value.problem_id;
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
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

    problem_dto::testcase_count testcase_count_value;
    testcase_count_value.testcase_count = increase_result[0][0].as<std::int32_t>();
    return testcase_count_value;
}

std::expected<problem_dto::testcase_count, error_code> testcase_repository::decrease_testcase_count(
    pqxx::transaction_base& transaction,
    const problem_dto::reference& problem_reference_value
){
    const auto decrease_result = transaction.exec(
        "UPDATE problem_statements "
        "SET testcase_count = testcase_count - 1, updated_at = NOW() "
        "WHERE problem_id = $1 AND testcase_count > 0 "
        "RETURNING testcase_count",
        pqxx::params{problem_reference_value.problem_id}
    );

    if(decrease_result.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    problem_dto::testcase_count testcase_count_value;
    testcase_count_value.testcase_count = decrease_result[0][0].as<std::int32_t>();
    return testcase_count_value;
}

std::expected<std::vector<problem_dto::testcase>, error_code> testcase_repository::list_testcases(
    pqxx::transaction_base& transaction,
    const problem_dto::reference& problem_reference_value
){
    const std::int64_t problem_id = problem_reference_value.problem_id;
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto testcases_query_result = transaction.exec(
        "SELECT testcase_id, testcase_order, testcase_input, testcase_output "
        "FROM problem_testcases "
        "WHERE problem_id = $1 "
        "ORDER BY testcase_order ASC",
        pqxx::params{problem_id}
    );

    std::vector<problem_dto::testcase> testcase_values;
    testcase_values.reserve(testcases_query_result.size());
    for(const auto& row : testcases_query_result){
        problem_dto::testcase testcase_value;
        testcase_value.id = row[0].as<std::int64_t>();
        testcase_value.order = row[1].as<std::int32_t>();
        testcase_value.input = row[2].as<std::string>();
        testcase_value.output = row[3].as<std::string>();
        testcase_values.push_back(std::move(testcase_value));
    }

    return testcase_values;
}

std::expected<void, error_code> testcase_repository::set_testcase(
    pqxx::transaction_base& transaction,
    const problem_dto::testcase_ref& testcase_reference_value,
    const problem_dto::testcase& testcase_value
){
    const std::int64_t problem_id = testcase_reference_value.problem_id;
    const std::int32_t testcase_order = testcase_reference_value.testcase_order;
    if(problem_id <= 0 || testcase_order <= 0){
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
            testcase_order,
            testcase_value.input,
            testcase_value.output
        }
    );

    if(update_result.affected_rows() == 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return {};
}

std::expected<void, error_code> testcase_repository::delete_testcase(
    pqxx::transaction_base& transaction,
    const problem_dto::reference& problem_reference_value
){
    const std::int64_t problem_id = problem_reference_value.problem_id;
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

    return {};
}

std::expected<void, error_code> testcase_repository::delete_all_testcases(
    pqxx::transaction_base& transaction,
    const problem_dto::reference& problem_reference_value
){
    const std::int64_t problem_id = problem_reference_value.problem_id;
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    transaction.exec(
        "DELETE FROM problem_testcases "
        "WHERE problem_id = $1",
        pqxx::params{problem_id}
    );

    return {};
}

std::expected<problem_dto::testcase_count, error_code> testcase_repository::clear_testcase_count(
    pqxx::transaction_base& transaction,
    const problem_dto::reference& problem_reference_value
){
    const std::int64_t problem_id = problem_reference_value.problem_id;
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto clear_result = transaction.exec(
        "UPDATE problem_statements "
        "SET testcase_count = 0, updated_at = NOW() "
        "WHERE problem_id = $1 "
        "RETURNING testcase_count",
        pqxx::params{problem_id}
    );

    if(clear_result.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    problem_dto::testcase_count testcase_count_value;
    testcase_count_value.testcase_count = clear_result[0][0].as<std::int32_t>();
    return testcase_count_value;
}
