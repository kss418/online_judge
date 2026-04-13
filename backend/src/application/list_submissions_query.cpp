#include "application/list_submissions_query.hpp"

#include "db_service/submission_query_service.hpp"

std::expected<submission_response_dto::summary_page, service_error> list_submissions_query::execute(
    db_connection& connection,
    const command& command_value
){
    return submission_query_service::list_submissions(
        connection,
        command_value.filter_value,
        command_value.viewer_user_id_opt
    );
}
