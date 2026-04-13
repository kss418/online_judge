#include "application/get_system_status_query.hpp"

#include "db_service/system_service.hpp"

std::expected<system_dto::status_response, service_error> get_system_status_query::execute(
    db_connection& connection,
    const command& command_value
){
    return system_service::get_status(
        connection,
        command_value.http_runtime_snapshot,
        command_value.judge_heartbeat_stale_after
    );
}
