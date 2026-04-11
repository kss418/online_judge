#include "judge_core/application/submission_lifecycle.hpp"

#include "judge_core/application/build_bundle.hpp"

#include <optional>
#include <string>
#include <utility>
#include <variant>

namespace{
    submission_decision make_compile_error_decision(
        const execution_report::testcase_execution& compile_execution_value
    ){
        execution_report::batch execution_report_value;
        execution_report_value.compile_failed = true;
        execution_report_value.executions.push_back(compile_execution_value);

        submission_decision submission_decision_value;
        submission_decision_value.verdict_summary.overall_verdict =
            judge_result::compile_error;
        submission_decision_value.verdict_summary.public_message_opt =
            std::string{"compile error"};
        if(!compile_execution_value.stderr_text.empty()){
            submission_decision_value.verdict_summary.internal_message_opt =
                compile_execution_value.stderr_text;
        }
        submission_decision_value.verdict_summary.score_opt = std::int16_t{0};
        submission_decision_value.execution_report_value =
            std::move(execution_report_value);
        return submission_decision_value;
    }

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

    std::string to_string(
        program_build::compile_resource_exceeded_reason reason_value
    ){
        switch(reason_value){
            case program_build::compile_resource_exceeded_reason::wall_clock:
                return "wall_clock";
            case program_build::compile_resource_exceeded_reason::signaled:
                return "signaled";
            case program_build::compile_resource_exceeded_reason::unknown:
                return "unknown";
        }

        return "unknown";
    }

    std::optional<std::string> select_compile_output(
        const execution_report::testcase_execution& compile_execution_value
    ){
        if(!compile_execution_value.stderr_text.empty()){
            return compile_execution_value.stderr_text;
        }
        if(!compile_execution_value.stdout_text.empty()){
            return compile_execution_value.stdout_text;
        }

        return std::nullopt;
    }

    submission_internal_dto::finalize_request make_build_resource_exceeded_finalize_request(
        const submission_domain_dto::leased_submission& leased_submission_value,
        const build_bundle::compile_resource_exceeded& compile_resource_exceeded_value
    ){
        return submission_internal_dto::make_finalize_request(
            leased_submission_value,
            submission_status::build_resource_exceeded,
            std::int16_t{0},
            select_compile_output(
                compile_resource_exceeded_value.compile_execution
            ),
            std::nullopt,
            std::nullopt,
            std::nullopt,
            to_string(compile_resource_exceeded_value.reason)
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

std::optional<submission_lifecycle::submission_completion>
submission_lifecycle::apply_build_policy(
    const submission_domain_dto::leased_submission& leased_submission_value,
    const build_bundle& build_result_value
) const{
    if(build_result_value.success()){
        return std::nullopt;
    }

    if(build_result_value.is_user_compile_error()){
        return make_decision_completion(
            leased_submission_value,
            make_compile_error_decision(
                build_result_value.user_compile_error_value().compile_execution
            )
        );
    }

    if(build_result_value.is_compile_resource_exceeded()){
        return submission_completion{
            finalize_command{
                make_build_resource_exceeded_finalize_request(
                    leased_submission_value,
                    build_result_value.compile_resource_exceeded_value()
                )
            }
        };
    }

    return make_infra_failure_completion(
        build_result_value.infra_failure().error
    );
}

submission_lifecycle::submission_completion
submission_lifecycle::make_decision_completion(
    const submission_domain_dto::leased_submission& leased_submission_value,
    const submission_decision& submission_decision_value
) const{
    return submission_completion{
        finalize_command{
            submission_decision_value.to_finalize_request(
                leased_submission_value
            )
        }
    };
}

submission_lifecycle::submission_completion
submission_lifecycle::make_infra_failure_completion(
    const judge_error& error_value
) const{
    return submission_completion{
        infra_failure_report{
            .error = error_value,
            .retry = decide_retry_directive(error_value),
        }
    };
}

std::expected<void, judge_error> submission_lifecycle::apply_completion(
    const submission_domain_dto::leased_submission& leased_submission_value,
    const submission_completion& submission_completion_value
){
    if(
        const auto finalize_command_opt =
            std::get_if<finalize_command>(&submission_completion_value.storage_)
    ){
        return apply_finalize_command(*finalize_command_opt);
    }

    return apply_infra_failure_report(
        leased_submission_value,
        std::get<infra_failure_report>(submission_completion_value.storage_)
    );
}

std::expected<void, judge_error> submission_lifecycle::apply_finalize_command(
    const finalize_command& finalize_command_value
){
    return judge_submission_facade_.finalize_submission(
        finalize_command_value.request
    );
}

std::expected<void, judge_error> submission_lifecycle::apply_infra_failure_report(
    const submission_domain_dto::leased_submission& leased_submission_value,
    const infra_failure_report& infra_failure_report_value
){
    if(
        infra_failure_report_value.retry ==
        retry_directive::requeue_immediately
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

submission_lifecycle::retry_directive
submission_lifecycle::decide_retry_directive(
    const judge_error& error_value
) const{
    if(error_value.code == judge_error_code::unavailable){
        return retry_directive::requeue_immediately;
    }

    return retry_directive::finalize_as_infra_failure;
}
