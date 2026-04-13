#include "application/get_submission_detail_query.hpp"

#include "db_service/submission_query_service.hpp"

std::expected<submission_response_dto::detail, service_error>
get_submission_detail_query::execute(
    db_connection& connection,
    const command& command_value
){
    return submission_query_service::get_submission_detail(
        connection,
        command_value.submission_id
    );
}
