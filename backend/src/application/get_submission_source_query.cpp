#include "application/get_submission_source_query.hpp"

#include "common/permission_util.hpp"
#include "db_service/submission_query_service.hpp"

get_submission_source_query::error get_submission_source_query::error::from_service_error(
    const service_error& service_error_value
){
    error error_value;
    error_value.kind_value = kind::service_error;
    error_value.service_error_value = service_error_value;
    return error_value;
}

get_submission_source_query::error get_submission_source_query::error::source_access_denied(){
    error error_value;
    error_value.kind_value = kind::source_access_denied;
    error_value.service_error_value = service_error::forbidden;
    return error_value;
}

bool get_submission_source_query::error::is_source_access_denied() const{
    return kind_value == kind::source_access_denied;
}

std::expected<submission_response_dto::source_detail, get_submission_source_query::error>
get_submission_source_query::execute(
    db_connection& connection,
    const command& command_value
){
    const auto source_detail_exp = submission_query_service::get_submission_source(
        connection,
        command_value.submission_id
    );
    if(!source_detail_exp){
        return std::unexpected(error::from_service_error(source_detail_exp.error()));
    }

    if(
        permission_util::has_admin_access(command_value.viewer_identity.permission_level) ||
        command_value.viewer_identity.user_id == source_detail_exp->user_id
    ){
        return std::move(*source_detail_exp);
    }

    return std::unexpected(error::source_access_denied());
}
