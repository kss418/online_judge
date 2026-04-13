#include "application/submission_command_action.hpp"

#include "db_service/submission_command_service.hpp"

std::expected<submission_response_dto::queued_response, service_error>
rejudge_submission_action::execute(
    db_connection& connection,
    const command& command_value
){
    return submission_command_service::rejudge_submission(
        connection,
        command_value.submission_id
    );
}
