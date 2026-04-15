#include "application/set_problem_statement_action.hpp"

#include "db_service/problem_content_service.hpp"

std::expected<problem_dto::mutation_result, service_error> set_problem_statement_action::execute(
    db_connection& connection,
    const command& command_value
){
    if(
        !problem_dto::is_valid(command_value.problem_reference_value) ||
        !problem_content_dto::is_valid(command_value.statement_value)
    ){
        return std::unexpected(service_error::validation_error);
    }

    return problem_content_service::set_statement(
        connection,
        command_value.problem_reference_value,
        command_value.statement_value
    );
}
