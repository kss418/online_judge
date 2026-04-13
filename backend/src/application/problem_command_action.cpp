#include "application/problem_command_action.hpp"

#include "db_service/problem_core_service.hpp"
#include "db_service/submission_service.hpp"

std::expected<problem_dto::created, service_error> create_problem_action::execute(
    db_connection& connection,
    const command& command_value
){
    return problem_core_service::create_problem(connection, command_value);
}

std::expected<problem_dto::mutation_result, service_error> update_problem_action::execute(
    db_connection& connection,
    const command& command_value
){
    return problem_core_service::update_problem(
        connection,
        command_value.problem_reference_value,
        command_value.update_request_value
    );
}

std::expected<void, service_error> delete_problem_action::execute(
    db_connection& connection,
    const command& command_value
){
    return problem_core_service::delete_problem(connection, command_value);
}

std::expected<void, service_error> rejudge_problem_action::execute(
    db_connection& connection,
    const command& command_value
){
    return submission_service::rejudge_problem(connection, command_value.problem_id);
}
