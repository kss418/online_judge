#include "db/testcase_service.hpp"

#include "db/problem_service_utility.hpp"
#include "db/testcase_service_utility.hpp"

#include <pqxx/pqxx>

#include <utility>

std::expected<std::int64_t, error_code> testcase_service::create_testcase(
    db_connection& connection,
    std::int64_t problem_id,
    const testcase& testcase_value
){
    if(!connection.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    try{
        pqxx::work transaction(connection.connection());
        const auto testcase_order_exp = testcase_service_utility::increase_testcase_count(
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
                testcase_value.testcase_input,
                testcase_value.testcase_output
            }
        );

        if(create_testcase_result.empty()){
            return std::unexpected(error_code::create(errno_error::unknown_error));
        }

        const auto version_exp = problem_service_utility::increase_version(transaction, problem_id);
        if(!version_exp){
            return std::unexpected(version_exp.error());
        }

        const std::int64_t testcase_id = create_testcase_result[0][0].as<std::int64_t>();
        transaction.commit();
        return testcase_id;
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<testcase, error_code> testcase_service::get_testcase(
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

        testcase testcase_value;
        testcase_value.testcase_id = testcase_query_result[0][0].as<std::int64_t>();
        testcase_value.testcase_order = testcase_query_result[0][1].as<std::int32_t>();
        testcase_value.testcase_input = testcase_query_result[0][2].as<std::string>();
        testcase_value.testcase_output = testcase_query_result[0][3].as<std::string>();
        return testcase_value;
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<std::vector<testcase>, error_code> testcase_service::list_testcases(
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
        const auto testcases_query_result = transaction.exec(
            "SELECT testcase_id, testcase_order, testcase_input, testcase_output "
            "FROM problem_testcases "
            "WHERE problem_id = $1 "
            "ORDER BY testcase_order ASC",
            pqxx::params{problem_id}
        );

        std::vector<testcase> testcase_values;
        testcase_values.reserve(testcases_query_result.size());
        for(const auto& row : testcases_query_result){
            testcase testcase_value;
            testcase_value.testcase_id = row[0].as<std::int64_t>();
            testcase_value.testcase_order = row[1].as<std::int32_t>();
            testcase_value.testcase_input = row[2].as<std::string>();
            testcase_value.testcase_output = row[3].as<std::string>();
            testcase_values.push_back(std::move(testcase_value));
        }

        return testcase_values;
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<void, error_code> testcase_service::set_testcase(
    db_connection& connection,
    std::int64_t problem_id,
    const testcase& testcase_value
){
    if(!connection.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(problem_id <= 0 || testcase_value.testcase_order <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    try{
        pqxx::work transaction(connection.connection());
        const auto update_result = transaction.exec(
            "UPDATE problem_testcases "
            "SET "
            "testcase_input = $3, "
            "testcase_output = $4 "
            "WHERE problem_id = $1 AND testcase_order = $2",
            pqxx::params{
                problem_id,
                testcase_value.testcase_order,
                testcase_value.testcase_input,
                testcase_value.testcase_output
            }
        );

        if(update_result.affected_rows() == 0){
            return std::unexpected(error_code::create(errno_error::invalid_argument));
        }

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

std::expected<void, error_code> testcase_service::delete_testcase(
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

        const auto testcase_count_exp = testcase_service_utility::decrease_testcase_count(
            transaction,
            problem_id
        );
        if(!testcase_count_exp){
            return std::unexpected(testcase_count_exp.error());
        }

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
