#include "application/list_user_wrong_problems_query.hpp"

#include "application/get_user_summary_query.hpp"
#include "db_service/problem_query_service.hpp"

std::expected<std::vector<problem_dto::summary>, service_error>
list_user_wrong_problems_query::execute(
    db_connection& connection,
    const command& command_value
){
    const auto user_summary_exp = get_user_summary_query::execute(
        connection,
        get_user_summary_query::command{.user_id = command_value.user_id}
    );
    if(!user_summary_exp){
        return std::unexpected(user_summary_exp.error());
    }

    return problem_query_service::list_user_wrong_problems(
        connection,
        command_value.user_id,
        command_value.viewer_user_id_opt
    );
}
