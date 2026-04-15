#include "application/replace_testcases_action.hpp"

#include "db_service/testcase_bulk_mutation_service.hpp"

std::expected<problem_dto::testcase_count_mutation_result, service_error>
replace_testcases_action::execute(
    db_connection& connection,
    const command& command_value
){
    if(!problem_dto::is_valid(command_value.problem_reference_value)){
        return std::unexpected(service_error::validation_error);
    }

    return testcase_bulk_mutation_service::replace_testcases(
        connection,
        command_value.problem_reference_value,
        command_value.testcase_values
    );
}
