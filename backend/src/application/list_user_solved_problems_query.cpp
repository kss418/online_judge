#include "application/list_user_solved_problems_query.hpp"

#include "db_service/problem_query_service.hpp"

std::expected<std::vector<problem_dto::summary>, service_error>
list_user_solved_problems_query::execute(
    db_connection& connection,
    const command& command_value
){
    if(
        command_value.user_id <= 0 ||
        (command_value.viewer_user_id_opt && *command_value.viewer_user_id_opt <= 0)
    ){
        return std::unexpected(service_error::validation_error);
    }

    return problem_query_service::list_user_solved_problems(
        connection,
        command_value.user_id,
        command_value.viewer_user_id_opt
    );
}
