#include "application/get_user_submission_ban_status_query.hpp"

#include "db_service/user_service.hpp"

std::expected<user_dto::submission_ban_status, service_error>
get_user_submission_ban_status_query::execute(
    db_connection& connection,
    const command& command_value
){
    return user_service::get_submission_ban_status(
        connection,
        command_value.user_id
    );
}
