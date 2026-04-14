#include "db_service/testcase_query_service.hpp"

#include "db_service/db_service_util.hpp"
#include "db_repository/testcase_query_repository.hpp"

std::expected<problem_dto::testcase, service_error> testcase_query_service::get_testcase(
    db_connection& connection,
    const problem_dto::testcase_ref& testcase_reference_value
){
    return db_service_util::with_retry_service_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<problem_dto::testcase, service_error> {
            return testcase_query_repository::get_testcase(
                transaction,
                testcase_reference_value
            );
        }
    );
}

std::expected<problem_dto::testcase_count, service_error> testcase_query_service::get_testcase_count(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value
){
    return db_service_util::with_retry_service_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<problem_dto::testcase_count, service_error> {
            return testcase_query_repository::get_testcase_count(
                transaction,
                problem_reference_value
            );
        }
    );
}

std::expected<std::vector<problem_dto::testcase>, service_error>
testcase_query_service::list_testcases(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value
){
    return db_service_util::with_retry_service_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<std::vector<problem_dto::testcase>, service_error> {
            return testcase_query_repository::list_testcases(
                transaction,
                problem_reference_value
            );
        }
    );
}

std::expected<std::vector<problem_dto::testcase_summary>, service_error>
testcase_query_service::list_testcase_summaries(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value
){
    return db_service_util::with_retry_service_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<std::vector<problem_dto::testcase_summary>, service_error> {
            return testcase_query_repository::list_testcase_summaries(
                transaction,
                problem_reference_value
            );
        }
    );
}
