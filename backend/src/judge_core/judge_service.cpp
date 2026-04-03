#include "judge_core/judge_service.hpp"

#include "db_service/submission_service.hpp"

namespace judge_service{
    std::expected<std::optional<submission_dto::queued_submission>, judge_error>
    poll_next_submission(
        db_connection& connection,
        submission_event_listener& submission_event_listener,
        std::chrono::seconds lease_duration,
        std::chrono::milliseconds notification_wait_timeout
    ){
        submission_dto::lease_request lease_request_value;
        lease_request_value.lease_duration = lease_duration;

        auto queued_submission_opt_exp = submission_service::lease_submission(
            connection,
            lease_request_value
        );
        if(!queued_submission_opt_exp){
            return std::unexpected(judge_error{queued_submission_opt_exp.error()});
        }

        if(queued_submission_opt_exp->has_value()){
            return queued_submission_opt_exp;
        }

        const auto wait_submission_notification_exp =
            submission_event_listener.wait_submission_notification(
                notification_wait_timeout
            );
        if(!wait_submission_notification_exp){
            return std::unexpected(
                judge_error{wait_submission_notification_exp.error()}
            );
        }

        return std::optional<submission_dto::queued_submission>{};
    }
}
