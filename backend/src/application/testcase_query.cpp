#include "application/testcase_query.hpp"

#include "db_service/testcase_service.hpp"

std::expected<problem_dto::testcase, service_error> get_testcase_query::execute(
    db_connection& connection,
    const command& command_value
){
    return testcase_service::get_testcase(connection, command_value);
}

std::expected<std::vector<problem_dto::testcase_summary>, service_error>
list_testcase_summaries_query::execute(
    db_connection& connection,
    const command& command_value
){
    return testcase_service::list_testcase_summaries(connection, command_value);
}
