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

std::expected<void, judge_error> submission_lifecycle::finalize(
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

std::expected<void, judge_error> submission_lifecycle::requeue(
    const submission_dto::queued_submission& queued_submission_value,
    const judge_error& error_value
){
    return judge_submission_facade_.requeue_submission_immediately(
        queued_submission_value.submission_id,
        to_string(error_value)
    );
}
