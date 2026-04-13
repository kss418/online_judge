#include "application/problem_content_query.hpp"

#include "db_service/problem_content_service.hpp"

std::expected<problem_content_dto::limits, service_error> get_problem_limits_query::execute(
    db_connection& connection,
    const command& command_value
){
    return problem_content_service::get_limits(connection, command_value);
}

std::expected<std::vector<problem_content_dto::sample>, service_error>
list_problem_samples_query::execute(
    db_connection& connection,
    const command& command_value
){
    return problem_content_service::list_samples(connection, command_value);
}
