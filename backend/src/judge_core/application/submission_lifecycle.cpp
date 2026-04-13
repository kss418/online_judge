#include "judge_core/application/submission_lifecycle.hpp"

#include <string>
#include <utility>

namespace{
    submission_internal_dto::finalize_request make_infra_failure_finalize_request(
        const submission_domain_dto::leased_submission& leased_submission_value,
        std::string reason
    ){
        return submission_internal_dto::make_finalize_request(
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
    const submission_domain_dto::leased_submission& leased_submission_value
){
    return judge_submission_facade_.mark_judging(
        leased_submission_value
    );
}

std::expected<void, judge_error> submission_lifecycle::apply_completion(
    const submission_domain_dto::leased_submission& leased_submission_value,
    const submission_completion& submission_completion_value
){
    if(const auto finalize_command_opt = submission_completion_value.finalize_command_opt()){
        return apply_finalize_command(*finalize_command_opt);
    }

    return apply_infra_failure_report(
        leased_submission_value,
        *submission_completion_value.infra_failure_report_opt()
    );
}

std::expected<void, judge_error> submission_lifecycle::apply_finalize_command(
    const submission_completion::finalize_command& finalize_command_value
){
    return judge_submission_facade_.finalize_submission(
        finalize_command_value.request
    );
}

std::expected<void, judge_error> submission_lifecycle::apply_infra_failure_report(
    const submission_domain_dto::leased_submission& leased_submission_value,
    const submission_completion::infra_failure_report&
        infra_failure_report_value
){
    if(
        infra_failure_report_value.retry ==
        submission_completion::retry_directive::requeue_immediately
    ){
        return requeue_submission(
            leased_submission_value,
            infra_failure_report_value.error
        );
    }

    const submission_internal_dto::finalize_request finalize_request_value =
        make_infra_failure_finalize_request(
            leased_submission_value,
            to_string(infra_failure_report_value.error)
        );
    return judge_submission_facade_.finalize_submission(finalize_request_value);
}

std::expected<void, judge_error> submission_lifecycle::requeue_submission(
    const submission_domain_dto::leased_submission& leased_submission_value,
    const judge_error& error_value
){
    return judge_submission_facade_.requeue_submission_immediately(
        leased_submission_value,
        to_string(error_value)
    );
}
