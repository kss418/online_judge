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

    submission_dto::finalize_request make_build_resource_exceeded_finalize_request(
        const submission_dto::leased_submission& leased_submission_value,
        const build_bundle::compile_resource_exceeded& compile_resource_exceeded_value
    ){
        return submission_dto::make_finalize_request(
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
    const submission_dto::leased_submission& leased_submission_value
){
    return judge_submission_facade_.mark_judging(
        leased_submission_value
    );
}

std::expected<void, judge_error> submission_lifecycle::complete(
    const submission_dto::leased_submission& leased_submission_value,
    completion_outcome submission_outcome_value
){
    if(const auto decision_opt = std::get_if<submission_decision>(&submission_outcome_value)){
        return finalize_judged_submission(
            leased_submission_value,
            *decision_opt
        );
    }

    if(
        const auto finalize_request_opt =
            std::get_if<submission_dto::finalize_request>(&submission_outcome_value)
    ){
        return finalize_direct_submission(*finalize_request_opt);
    }

    return handle_infra_failure(leased_submission_value, std::get<judge_error>(submission_outcome_value));
}

submission_lifecycle::build_policy_outcome
submission_lifecycle::apply_build_policy(
    const submission_dto::leased_submission& leased_submission_value,
    const build_bundle& build_result_value
) const{
    if(build_result_value.success()){
        return std::monostate{};
    }

    if(build_result_value.is_user_compile_error()){
        return make_compile_error_decision(
            build_result_value.user_compile_error_value().compile_execution
        );
    }

    if(build_result_value.is_compile_resource_exceeded()){
        return make_build_resource_exceeded_finalize_request(
            leased_submission_value,
            build_result_value.compile_resource_exceeded_value()
        );
    }

    return build_result_value.infra_failure().error;
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

std::expected<void, judge_error> submission_lifecycle::finalize_direct_submission(
    const submission_dto::finalize_request& finalize_request_value
){
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
