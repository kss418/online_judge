#include "application/set_problem_limits_action.hpp"

#include "db_service/problem_content_service.hpp"
#include "db_service/problem_core_service.hpp"

std::expected<problem_dto::mutation_result, service_error> set_problem_limits_action::execute(
    db_connection& connection,
    const command& command_value
){
    if(
        !problem_dto::is_valid(command_value.problem_reference_value) ||
        !problem_content_dto::is_valid(command_value.limits_value)
    ){
        return std::unexpected(service_error::validation_error);
    }

    const auto ensure_problem_exists_exp = problem_core_service::ensure_problem_exists(
        connection,
        command_value.problem_reference_value
    );
    if(!ensure_problem_exists_exp){
        return std::unexpected(ensure_problem_exists_exp.error());
    }

    return problem_content_service::set_limits(
        connection,
        command_value.problem_reference_value,
        command_value.limits_value
    );
}
