#include "db_service/testcase_service.hpp"

#include "db_util/problem_util.hpp"
#include "db_util/testcase_util.hpp"

#include <pqxx/pqxx>

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
        const auto ensure_statement_exp = problem_util::ensure_statement_row(
            transaction,
            problem_id
        );
        if(!ensure_statement_exp){
            return std::unexpected(ensure_statement_exp.error());
        }

        const auto testcase_order_exp = testcase_util::increase_testcase_count(
            transaction,
            problem_id
        );
        if(!testcase_order_exp){
            return std::unexpected(testcase_order_exp.error());
        }

        const auto created_testcase_exp = testcase_util::create_testcase(
            transaction,
            problem_id,
            *testcase_order_exp,
            testcase_value
        );
        if(!created_testcase_exp){
            return std::unexpected(created_testcase_exp.error());
        }

        const auto version_exp = problem_util::increase_version(transaction, problem_id);
        if(!version_exp){
            return std::unexpected(version_exp.error());
        }

        transaction.commit();
        return *created_testcase_exp;
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
        const auto testcase_exp = testcase_util::get_testcase(
            transaction,
            problem_id,
            testcase_order
        );
        if(!testcase_exp){
            return std::unexpected(testcase_exp.error());
        }

        transaction.commit();
        return *testcase_exp;
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
        const auto testcase_count_exp = testcase_util::get_testcase_count(
            transaction,
            problem_id
        );
        if(!testcase_count_exp){
            return std::unexpected(testcase_count_exp.error());
        }

        transaction.commit();
        return *testcase_count_exp;
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}
