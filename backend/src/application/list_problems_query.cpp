#include "application/list_problems_query.hpp"

#include "db_service/problem_core_service.hpp"

#include <utility>

std::expected<list_problems_query::result, service_error> list_problems_query::execute(
    db_connection& connection,
    const command& command_value
){
    const auto summary_values_exp = problem_core_service::list_problems(
        connection,
        command_value.filter_value,
        command_value.viewer_user_id_opt
    );
    if(!summary_values_exp){
        return std::unexpected(summary_values_exp.error());
    }

    const auto total_problem_count_exp = problem_core_service::count_problems(
        connection,
        command_value.filter_value,
        command_value.viewer_user_id_opt
    );
    if(!total_problem_count_exp){
        return std::unexpected(total_problem_count_exp.error());
    }

    result result_value;
    result_value.summary_values = std::move(*summary_values_exp);
    result_value.total_problem_count = *total_problem_count_exp;
    return result_value;
}
