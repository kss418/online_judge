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
    std::int64_t submission_id
){
    return judge_submission_facade_.mark_judging(submission_id);
}

std::expected<void, judge_error> submission_lifecycle::finalize_submission(
    std::int64_t submission_id,
    judge_result result,
    const execution_report::batch& execution_report_value
){
    const submission_dto::finalize_request finalize_request_value =
        finalize_submission_mapper::make_finalize_request(
            submission_id,
            result,
            execution_report_value
        );

    return judge_submission_facade_.finalize_submission(finalize_request_value);
}

std::expected<void, judge_error> submission_lifecycle::requeue_submission(
    std::int64_t submission_id,
    std::string reason
){
    return judge_submission_facade_.requeue_submission_immediately(
        submission_id,
        std::move(reason)
    );
}
