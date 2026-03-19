#include "db_service/testcase_service.hpp"

#include "db_util/problem_util.hpp"
#include "db_util/testcase_util.hpp"

#include <pqxx/pqxx>

#include <utility>

std::expected<problem_dto::testcase, error_code> testcase_service::create_testcase(
    db_connection& connection,
    std::int64_t problem_id,
    const problem_dto::testcase& testcase_value
){
    if(!connection.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    try{
        pqxx::work transaction(connection.connection());
        const auto testcase_order_exp = testcase_util::increase_testcase_count(
            transaction,
            problem_id
        );
        if(!testcase_order_exp){
            return std::unexpected(testcase_order_exp.error());
        }

        const std::int32_t testcase_order = testcase_order_exp.value();
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

        const auto version_exp = problem_util::increase_version(transaction, problem_id);
        if(!version_exp){
            return std::unexpected(version_exp.error());
        }

        problem_dto::testcase created_testcase_value = testcase_value;
        created_testcase_value.id = create_testcase_result[0][0].as<std::int64_t>();
        created_testcase_value.order = testcase_order;
        transaction.commit();
        return created_testcase_value;
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<problem_dto::testcase, error_code> testcase_service::get_testcase(
    db_connection& connection,
    std::int64_t problem_id,
    std::int32_t testcase_order
){
    if(!connection.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(problem_id <= 0 || testcase_order <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    try{
        pqxx::work transaction(connection.connection());
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
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<std::int32_t, error_code> testcase_service::get_testcase_count(
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
        const auto testcase_count_query_result = transaction.exec(
            "SELECT COUNT(*) "
            "FROM problem_testcases "
            "WHERE problem_id = $1",
            pqxx::params{problem_id}
        );

        if(testcase_count_query_result.empty()){
            return std::unexpected(error_code::create(errno_error::unknown_error));
        }

        return testcase_count_query_result[0][0].as<std::int32_t>();
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}
