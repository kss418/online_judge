#include "judge_core/application/submission_completion_policy.hpp"

#include "judge_core/application/build_bundle.hpp"

#include <optional>
#include <string>
#include <utility>

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

    submission_internal_dto::finalize_request
    make_build_resource_exceeded_finalize_request(
        const submission_domain_dto::leased_submission& leased_submission_value,
        const build_bundle::compile_resource_exceeded&
            compile_resource_exceeded_value
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

std::optional<submission_completion>
submission_completion_policy::make_build_completion(
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
        return submission_completion::from_finalize_command(
            submission_completion::finalize_command{
                make_build_resource_exceeded_finalize_request(
                    leased_submission_value,
                    build_result_value.compile_resource_exceeded_value()
                )
            }
        );
    }

    return make_infra_failure_completion(
        build_result_value.infra_failure().error
    );
}

submission_completion submission_completion_policy::make_decision_completion(
    const submission_domain_dto::leased_submission& leased_submission_value,
    const submission_decision& submission_decision_value
) const{
    return submission_completion::from_finalize_command(
        submission_completion::finalize_command{
            submission_decision_value.to_finalize_request(
                leased_submission_value
            )
        }
    );
}

submission_completion submission_completion_policy::make_infra_failure_completion(
    const judge_error& error_value
) const{
    return submission_completion::from_infra_failure_report(
        submission_completion::infra_failure_report{
            .error = error_value,
            .retry = decide_retry_directive(error_value),
        }
    );
}

submission_completion::retry_directive
submission_completion_policy::decide_retry_directive(
    const judge_error& error_value
) const{
    if(error_value.code == judge_error_code::unavailable){
        return submission_completion::retry_directive::requeue_immediately;
    }

    return submission_completion::retry_directive::finalize_as_infra_failure;
}
