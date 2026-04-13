#include "application/testcase_action.hpp"

#include "db_service/testcase_service.hpp"

std::expected<problem_dto::testcase_mutation_result, service_error>
create_testcase_action::execute(
    db_connection& connection,
    const command& command_value
){
    return testcase_service::create_testcase(
        connection,
        command_value.problem_reference_value,
        command_value.testcase_value
    );
}

std::expected<problem_dto::testcase_mutation_result, service_error>
update_testcase_action::execute(
    db_connection& connection,
    const command& command_value
){
    return testcase_service::set_testcase_and_get(
        connection,
        command_value.testcase_reference_value,
        command_value.testcase_value
    );
}

std::expected<problem_dto::mutation_result, service_error> move_testcase_action::execute(
    db_connection& connection,
    const command& command_value
){
    return testcase_service::move_testcase(
        connection,
        command_value.testcase_reference_value,
        command_value.target_testcase_order
    );
}

std::expected<problem_dto::mutation_result, service_error> delete_testcase_action::execute(
    db_connection& connection,
    const command& command_value
){
    return testcase_service::delete_testcase(connection, command_value);
}

std::expected<problem_dto::mutation_result, service_error>
delete_all_testcases_action::execute(
    db_connection& connection,
    const command& command_value
){
    return testcase_service::delete_all_testcases(connection, command_value);
}
