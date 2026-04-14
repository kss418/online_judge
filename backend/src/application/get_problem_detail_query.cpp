#include "application/get_problem_detail_query.hpp"

#include "db_service/problem_query_service.hpp"

std::expected<problem_dto::detail, service_error> get_problem_detail_query::execute(
    db_connection& connection,
    const command& command_value
){
    if(!problem_dto::is_valid(command_value.problem_reference_value)){
        return std::unexpected(service_error::validation_error);
    }

    return problem_query_service::get_problem_detail(
        connection,
        command_value.problem_reference_value,
        command_value.viewer_user_id_opt
    );
}
