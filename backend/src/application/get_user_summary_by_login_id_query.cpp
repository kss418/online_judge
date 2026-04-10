#include "application/get_user_summary_by_login_id_query.hpp"

#include "db_service/user_service.hpp"

std::expected<user_dto::summary, service_error>
get_user_summary_by_login_id_query::execute(
    db_connection& connection,
    const command& command_value
){
    return user_service::get_summary_by_login_id(
        connection,
        command_value.user_login_id
    );
}
