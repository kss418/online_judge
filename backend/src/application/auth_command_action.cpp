#include "application/auth_command_action.hpp"

#include "db_service/auth_service.hpp"
#include "db_service/login_service.hpp"

std::expected<auth_dto::session, service_error> sign_up_action::execute(
    db_connection& connection,
    const command& command_value
){
    return login_service::sign_up(connection, command_value);
}

std::expected<auth_dto::session, service_error> login_action::execute(
    db_connection& connection,
    const command& command_value
){
    return login_service::login(connection, command_value);
}

std::expected<void, service_error> renew_auth_token_action::execute(
    db_connection& connection,
    const command& command_value
){
    return auth_service::renew_token(connection, command_value);
}

std::expected<void, service_error> logout_action::execute(
    db_connection& connection,
    const command& command_value
){
    return auth_service::revoke_token(connection, command_value);
}
