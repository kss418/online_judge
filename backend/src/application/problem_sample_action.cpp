#include "application/problem_sample_action.hpp"

#include "db_service/problem_content_service.hpp"

std::expected<problem_dto::sample_mutation_result, service_error>
create_problem_sample_action::execute(
    db_connection& connection,
    const command& command_value
){
    return problem_content_service::create_sample(
        connection,
        command_value.problem_reference_value,
        problem_content_dto::sample{}
    );
}

std::expected<problem_dto::sample_mutation_result, service_error>
update_problem_sample_action::execute(
    db_connection& connection,
    const command& command_value
){
    return problem_content_service::set_sample_and_get(
        connection,
        command_value.sample_reference_value,
        command_value.sample_value
    );
}

std::expected<problem_dto::mutation_result, service_error>
delete_problem_sample_action::execute(
    db_connection& connection,
    const command& command_value
){
    return problem_content_service::delete_sample(connection, command_value);
}
