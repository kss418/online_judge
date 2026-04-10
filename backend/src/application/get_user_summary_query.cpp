#include "application/get_user_summary_query.hpp"

#include "db_service/user_service.hpp"

std::expected<user_dto::summary, service_error> get_user_summary_query::execute(
    db_connection& connection,
    const command& command_value
){
    return user_service::get_summary(
        connection,
        command_value.user_id
    );
}
