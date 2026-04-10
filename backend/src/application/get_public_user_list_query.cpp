#include "application/get_public_user_list_query.hpp"

#include "db_service/user_service.hpp"

std::expected<user_dto::list, service_error> get_public_user_list_query::execute(
    db_connection& connection,
    const command& command_value
){
    return user_service::get_public_list(
        connection,
        command_value.filter_value
    );
}
