#include "judge_core/application/finalize_submission_mapper.hpp"
#include "judge_core/application/submission_lifecycle.hpp"

#include <utility>

std::expected<submission_lifecycle, judge_error> submission_lifecycle::create(
    judge_submission_facade judge_submission_facade_value
){
    return submission_lifecycle(std::move(judge_submission_facade_value));
}

submission_lifecycle::submission_lifecycle(
    judge_submission_facade judge_submission_facade_value
) :
    judge_submission_facade_(std::move(judge_submission_facade_value)){}

submission_lifecycle::submission_lifecycle(
    submission_lifecycle&& other
) noexcept = default;

submission_lifecycle& submission_lifecycle::operator=(
    submission_lifecycle&& other
) noexcept = default;

submission_lifecycle::~submission_lifecycle() = default;

std::expected<void, judge_error> submission_lifecycle::mark_judging(
    const submission_dto::queued_submission& queued_submission_value
){
    return judge_submission_facade_.mark_judging(
        queued_submission_value.submission_id
    );
}

std::expected<void, judge_error> submission_lifecycle::complete(
    const submission_dto::queued_submission& queued_submission_value,
    std::expected<submission_decision, judge_error> submission_outcome_value
){
    if(submission_outcome_value){
        return finalize_judged_submission(
            queued_submission_value,
            *submission_outcome_value
        );
    }

    return handle_infra_failure(
        queued_submission_value,
        submission_outcome_value.error()
    );
}

std::expected<void, judge_error> submission_lifecycle::finalize_judged_submission(
    const submission_dto::queued_submission& queued_submission_value,
    const submission_decision& submission_decision_value
){
    const submission_dto::finalize_request finalize_request_value =
        finalize_submission_mapper::make_finalize_request(
            queued_submission_value.submission_id,
            submission_decision_value.judge_result_value,
            submission_decision_value.execution_report_value
        );

    return judge_submission_facade_.finalize_submission(finalize_request_value);
}

std::expected<void, judge_error> submission_lifecycle::handle_infra_failure(
    const submission_dto::queued_submission& queued_submission_value,
    const judge_error& error_value
){
    if(
        decide_retry_directive(error_value) ==
        retry_directive::requeue_immediately
    ){
        return requeue_submission(
            queued_submission_value,
            error_value
        );
    }

    const submission_dto::finalize_request finalize_request_value =
        finalize_submission_mapper::make_infra_failure_finalize_request(
            queued_submission_value.submission_id,
            to_string(error_value)
        );
    return judge_submission_facade_.finalize_submission(finalize_request_value);
}

std::expected<void, judge_error> submission_lifecycle::requeue_submission(
    const submission_dto::queued_submission& queued_submission_value,
    const judge_error& error_value
){
    return judge_submission_facade_.requeue_submission_immediately(
        queued_submission_value.submission_id,
        to_string(error_value)
    );
}

submission_lifecycle::retry_directive
submission_lifecycle::decide_retry_directive(
    const judge_error& error_value
) const{
    if(error_value.code == judge_error_code::unavailable){
        return retry_directive::requeue_immediately;
    }

    return retry_directive::finalize_as_infra_failure;
}
