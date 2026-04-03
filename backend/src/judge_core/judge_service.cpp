#include "judge_core/judge_service.hpp"

#include "db_service/submission_service.hpp"

namespace judge_service{
    std::expected<std::optional<submission_dto::queued_submission>, judge_error>
    poll_next_submission(
        db_connection& connection,
        submission_queue_source& submission_queue_source,
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
            submission_queue_source.wait_submission_notification(
                notification_wait_timeout
            );
        if(!wait_submission_notification_exp){
            return std::unexpected(
                judge_error{wait_submission_notification_exp.error()}
            );
        }

        return std::optional<submission_dto::queued_submission>{};
    }

    std::expected<void, judge_error> mark_judging(
        db_connection& connection,
        std::int64_t submission_id
    ){
        const auto mark_judging_exp = submission_service::mark_judging(
            connection,
            submission_id
        );
        if(!mark_judging_exp){
            return std::unexpected(judge_error{mark_judging_exp.error()});
        }

        return {};
    }

    std::expected<void, judge_error> finalize_submission(
        db_connection& connection,
        const submission_dto::finalize_request& finalize_request_value
    ){
        const auto finalize_submission_exp = submission_service::finalize_submission(
            connection,
            finalize_request_value
        );
        if(!finalize_submission_exp){
            return std::unexpected(judge_error{finalize_submission_exp.error()});
        }

        return {};
    }

    std::expected<void, judge_error> requeue_submission_immediately(
        db_connection& connection,
        std::int64_t submission_id,
        std::optional<std::string> reason_opt
    ){
        const auto requeue_submission_exp =
            submission_service::requeue_submission_immediately(
                connection,
                submission_id,
                std::move(reason_opt)
            );
        if(!requeue_submission_exp){
            return std::unexpected(judge_error{requeue_submission_exp.error()});
        }

        return {};
    }
}
