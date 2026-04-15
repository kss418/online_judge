#include "application/get_user_submission_statistics_query.hpp"

#include "db_service/user_service.hpp"
#include "db_service/user_statistics_service.hpp"

std::expected<user_statistics_dto::submission_statistics, service_error>
get_user_submission_statistics_query::execute(
    db_connection& connection,
    const command& command_value
){
    const auto user_summary_exp = user_service::get_summary(
        connection,
        command_value.user_id
    );
    if(!user_summary_exp){
        return std::unexpected(user_summary_exp.error());
    }

    return user_statistics_service::get_submission_statistics(
        connection,
        command_value.user_id
    );
}
