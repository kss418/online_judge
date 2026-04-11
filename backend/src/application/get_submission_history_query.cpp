#include "application/get_submission_history_query.hpp"

#include "db_service/submission_service.hpp"

std::expected<submission_response_dto::history_list, service_error>
get_submission_history_query::execute(
    db_connection& connection,
    const command& command_value
){
    return submission_service::get_submission_history(
        connection,
        command_value.submission_id
    );
}
