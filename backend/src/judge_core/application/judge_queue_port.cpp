#include "judge_core/application/judge_queue_port.hpp"

#include "db_service/submission_service.hpp"

#include <utility>

std::expected<judge_queue_port, judge_error> judge_queue_port::create(
    const db_connection_config& db_config
){
    auto lease_db_connection_exp = db_connection::create(db_config);
    if(!lease_db_connection_exp){
        return std::unexpected(judge_error{lease_db_connection_exp.error()});
    }

    auto queue_source_connection_exp = db_connection::create(db_config);
    if(!queue_source_connection_exp){
        return std::unexpected(judge_error{queue_source_connection_exp.error()});
    }

    auto submission_queue_source_exp = submission_queue_source::create(
        std::move(*queue_source_connection_exp)
    );
    if(!submission_queue_source_exp){
        return std::unexpected(judge_error{submission_queue_source_exp.error()});
    }

    return judge_queue_port(
        std::move(*lease_db_connection_exp),
        std::move(*submission_queue_source_exp)
    );
}

judge_queue_port::judge_queue_port(
    db_connection lease_db_connection,
    submission_queue_source submission_queue_source_value
) :
    lease_db_connection_(std::move(lease_db_connection)),
    submission_queue_source_(std::move(submission_queue_source_value)){}

judge_queue_port::judge_queue_port(
    judge_queue_port&& other
) noexcept = default;

judge_queue_port& judge_queue_port::operator=(
    judge_queue_port&& other
) noexcept = default;

judge_queue_port::~judge_queue_port() = default;

std::expected<std::optional<submission_dto::queued_submission>, judge_error>
judge_queue_port::poll_next_submission(
    std::chrono::seconds lease_duration,
    std::chrono::milliseconds notification_wait_timeout
){
    submission_dto::lease_request lease_request_value;
    lease_request_value.lease_duration = lease_duration;

    auto queued_submission_opt_exp = submission_service::lease_submission(
        lease_db_connection_,
        lease_request_value
    );
    if(!queued_submission_opt_exp){
        return std::unexpected(judge_error{queued_submission_opt_exp.error()});
    }
    if(queued_submission_opt_exp->has_value()){
        return std::move(*queued_submission_opt_exp);
    }

    const auto wait_submission_notification_exp =
        submission_queue_source_.wait_submission_notification(
            notification_wait_timeout
        );
    if(!wait_submission_notification_exp){
        return std::unexpected(
            judge_error{wait_submission_notification_exp.error()}
        );
    }

    return std::optional<submission_dto::queued_submission>{};
}
