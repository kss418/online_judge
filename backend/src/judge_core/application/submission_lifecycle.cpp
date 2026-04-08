#include "judge_core/application/submission_lifecycle.hpp"

#include <optional>
#include <utility>

namespace{
    submission_dto::finalize_request make_infra_failure_finalize_request(
        const submission_dto::leased_submission& leased_submission_value,
        std::string reason
    ){
        return submission_dto::make_finalize_request(
            leased_submission_value,
            submission_status::infra_failure,
            std::nullopt,
            std::nullopt,
            std::nullopt,
            std::nullopt,
            std::nullopt,
            std::move(reason)
        );
    }
}

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
    const submission_dto::leased_submission& leased_submission_value
){
    return judge_submission_facade_.mark_judging(
        leased_submission_value
    );
}

std::expected<void, judge_error> submission_lifecycle::complete(
    const submission_dto::leased_submission& leased_submission_value,
    std::expected<submission_decision, judge_error> submission_outcome_value
){
    if(submission_outcome_value){
        return finalize_judged_submission(
            leased_submission_value,
            *submission_outcome_value
        );
    }

    return handle_infra_failure(
        leased_submission_value,
        submission_outcome_value.error()
    );
}

std::expected<void, judge_error> submission_lifecycle::finalize_judged_submission(
    const submission_dto::leased_submission& leased_submission_value,
    const submission_decision& submission_decision_value
){
    const submission_dto::finalize_request finalize_request_value =
        submission_decision_value.to_finalize_request(
            leased_submission_value
        );

    return judge_submission_facade_.finalize_submission(finalize_request_value);
}

std::expected<void, judge_error> submission_lifecycle::handle_infra_failure(
    const submission_dto::leased_submission& leased_submission_value,
    const judge_error& error_value
){
    if(
        decide_retry_directive(error_value) ==
        retry_directive::requeue_immediately
    ){
        return requeue_submission(
            leased_submission_value,
            error_value
        );
    }

    const submission_dto::finalize_request finalize_request_value =
        make_infra_failure_finalize_request(
            leased_submission_value,
            to_string(error_value)
        );
    return judge_submission_facade_.finalize_submission(finalize_request_value);
}

std::expected<void, judge_error> submission_lifecycle::requeue_submission(
    const submission_dto::leased_submission& leased_submission_value,
    const judge_error& error_value
){
    return judge_submission_facade_.requeue_submission_immediately(
        leased_submission_value,
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
