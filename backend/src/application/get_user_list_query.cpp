#include "application/get_user_list_query.hpp"

#include "db_service/auth_service.hpp"

std::expected<auth_dto::user_summary_list, service_error> get_user_list_query::execute(
    db_connection& connection,
    const command&
){
    return auth_service::get_user_list(connection);
}
