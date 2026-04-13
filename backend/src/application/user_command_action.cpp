#include "application/user_command_action.hpp"

#include "db_service/auth_service.hpp"
#include "db_service/user_service.hpp"

std::expected<void, service_error> update_user_permission_action::execute(
    db_connection& connection,
    const command& command_value
){
    return auth_service::update_permission_level(
        connection,
        command_value.user_id,
        command_value.permission_level
    );
}

std::expected<user_dto::submission_ban, service_error>
create_user_submission_ban_action::execute(
    db_connection& connection,
    const command& command_value
){
    return user_service::create_submission_ban(
        connection,
        command_value.user_id,
        command_value.duration_minutes
    );
}

std::expected<void, service_error> clear_user_submission_ban_action::execute(
    db_connection& connection,
    const command& command_value
){
    return user_service::clear_submission_banned_until(
        connection,
        command_value.user_id
    );
}
