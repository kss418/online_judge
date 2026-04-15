#include "application/get_user_submission_statistics_query.hpp"

#include "db_service/user_statistics_service.hpp"

std::expected<user_statistics_dto::submission_statistics, service_error>
get_user_submission_statistics_query::execute(
    db_connection& connection,
    const command& command_value
){
    if(command_value.user_id <= 0){
        return std::unexpected(service_error::validation_error);
    }

    return user_statistics_service::get_submission_statistics(
        connection,
        command_value.user_id
    );
}
