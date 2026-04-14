#include "application/create_submission_action.hpp"

#include "db_service/problem_query_service.hpp"
#include "db_service/submission_command_service.hpp"

#include <utility>

create_submission_action::error create_submission_action::error::from_service_error(
    const service_error& service_error_value
){
    error error_value;
    error_value.kind_value = kind::service_error;
    error_value.service_error_value = service_error_value;
    return error_value;
}

create_submission_action::error create_submission_action::error::submission_banned(){
    error error_value;
    error_value.kind_value = kind::submission_banned;
    error_value.service_error_value = service_error::forbidden;
    return error_value;
}

bool create_submission_action::error::is_submission_banned() const{
    return kind_value == kind::submission_banned;
}

std::expected<submission_response_dto::queued_response, create_submission_action::error>
create_submission_action::execute(
    db_connection& connection,
    const command& command_value
){
    if(!submission_internal_dto::is_valid(command_value)){
        return std::unexpected(error::from_service_error(service_error::validation_error));
    }

    const problem_dto::reference problem_reference_value{command_value.problem_id};
    const auto ensure_problem_exists_exp = problem_query_service::ensure_problem_exists(
        connection,
        problem_reference_value
    );
    if(!ensure_problem_exists_exp){
        return std::unexpected(error::from_service_error(
            ensure_problem_exists_exp.error()
        ));
    }

    const auto create_submission_exp = submission_command_service::create_submission(
        connection,
        command_value
    );
    if(!create_submission_exp){
        if(create_submission_exp.error() == service_error::forbidden){
            return std::unexpected(error::submission_banned());
        }

        return std::unexpected(error::from_service_error(
            create_submission_exp.error()
        ));
    }

    return std::move(*create_submission_exp);
}
