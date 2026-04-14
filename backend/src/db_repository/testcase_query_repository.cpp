#include "db_repository/testcase_query_repository.hpp"

#include <pqxx/pqxx>

#include <utility>

std::expected<problem_dto::testcase, repository_error> testcase_query_repository::get_testcase(
    pqxx::transaction_base& transaction,
    const problem_dto::testcase_ref& testcase_reference_value
){
    const std::int64_t problem_id = testcase_reference_value.problem_id;
    const std::int32_t testcase_order = testcase_reference_value.testcase_order;
    if(!problem_dto::is_valid(testcase_reference_value)){
        return std::unexpected(repository_error::invalid_reference);
    }

    const auto testcase_query_result = transaction.exec(
        "SELECT testcase_id, testcase_order, testcase_input, testcase_output "
        "FROM problem_testcases "
        "WHERE problem_id = $1 AND testcase_order = $2",
        pqxx::params{problem_id, testcase_order}
    );

    if(testcase_query_result.empty()){
        return std::unexpected(repository_error::not_found);
    }

    problem_dto::testcase testcase_value;
    testcase_value.id = testcase_query_result[0][0].as<std::int64_t>();
    testcase_value.order = testcase_query_result[0][1].as<std::int32_t>();
    testcase_value.input = testcase_query_result[0][2].as<std::string>();
    testcase_value.output = testcase_query_result[0][3].as<std::string>();
    return testcase_value;
}

std::expected<problem_dto::testcase_count, repository_error>
testcase_query_repository::get_testcase_count(
    pqxx::transaction_base& transaction,
    const problem_dto::reference& problem_reference_value
){
    const std::int64_t problem_id = problem_reference_value.problem_id;
    if(!problem_dto::is_valid(problem_reference_value)){
        return std::unexpected(repository_error::invalid_reference);
    }

    const auto testcase_count_query_result = transaction.exec(
        "SELECT COUNT(*) "
        "FROM problem_testcases "
        "WHERE problem_id = $1",
        pqxx::params{problem_id}
    );

    if(testcase_count_query_result.empty()){
        return std::unexpected(repository_error::internal);
    }

    problem_dto::testcase_count testcase_count_value;
    testcase_count_value.testcase_count = testcase_count_query_result[0][0].as<std::int32_t>();
    return testcase_count_value;
}

std::expected<std::vector<problem_dto::testcase>, repository_error>
testcase_query_repository::list_testcases(
    pqxx::transaction_base& transaction,
    const problem_dto::reference& problem_reference_value
){
    const std::int64_t problem_id = problem_reference_value.problem_id;
    if(!problem_dto::is_valid(problem_reference_value)){
        return std::unexpected(repository_error::invalid_reference);
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

std::expected<std::vector<problem_dto::testcase_summary>, repository_error>
testcase_query_repository::list_testcase_summaries(
    pqxx::transaction_base& transaction,
    const problem_dto::reference& problem_reference_value
){
    const std::int64_t problem_id = problem_reference_value.problem_id;
    if(!problem_dto::is_valid(problem_reference_value)){
        return std::unexpected(repository_error::invalid_reference);
    }

    const auto testcases_query_result = transaction.exec(
        "SELECT "
        "testcase_id, "
        "testcase_order, "
        "input_char_count, "
        "input_line_count, "
        "output_char_count, "
        "output_line_count "
        "FROM problem_testcases "
        "WHERE problem_id = $1 "
        "ORDER BY testcase_order ASC",
        pqxx::params{problem_id}
    );

    std::vector<problem_dto::testcase_summary> testcase_summary_values;
    testcase_summary_values.reserve(testcases_query_result.size());
    for(const auto& row : testcases_query_result){
        problem_dto::testcase_summary testcase_summary_value;
        testcase_summary_value.id = row[0].as<std::int64_t>();
        testcase_summary_value.order = row[1].as<std::int32_t>();
        testcase_summary_value.input_char_count = row[2].as<std::int32_t>();
        testcase_summary_value.input_line_count = row[3].as<std::int32_t>();
        testcase_summary_value.output_char_count = row[4].as<std::int32_t>();
        testcase_summary_value.output_line_count = row[5].as<std::int32_t>();
        testcase_summary_values.push_back(std::move(testcase_summary_value));
    }

    return testcase_summary_values;
}
