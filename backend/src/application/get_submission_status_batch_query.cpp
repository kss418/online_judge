#include "application/get_submission_status_batch_query.hpp"

#include "db_service/submission_service.hpp"

std::expected<std::vector<submission_response_dto::status_snapshot>, service_error>
get_submission_status_batch_query::execute(
    db_connection& connection,
    const command& command_value
){
    return submission_service::get_submission_status_snapshots(
        connection,
        command_value.submission_ids
    );
}
