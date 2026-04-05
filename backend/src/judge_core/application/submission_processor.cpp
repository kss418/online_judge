#include "judge_core/application/finalize_submission_mapper.hpp"
#include "judge_core/application/submission_processor.hpp"

#include <utility>

namespace{
    execution_report::testcase_execution make_testcase_execution(
        const program_build::compile_failure& compile_failure_value
    ){
        execution_report::testcase_execution testcase_execution_value;
        testcase_execution_value.exit_code = compile_failure_value.exit_code_;
        testcase_execution_value.stderr_text = compile_failure_value.stderr_text_;
        return testcase_execution_value;
    }

    execution_report::batch make_compile_failed_execution_report(
        const program_build::compile_failure& compile_failure_value
    ){
        execution_report::batch execution_report_value;
        execution_report_value.compile_failed = true;
        execution_report_value.executions.push_back(
            make_testcase_execution(compile_failure_value)
        );
        return execution_report_value;
    }
}

std::expected<submission_processor, judge_error> submission_processor::create(
    dependencies dependencies_value
){
    auto workspace_runner_exp = workspace_runner::create(
        std::move(dependencies_value.source_root_path)
    );
    if(!workspace_runner_exp){
        return std::unexpected(workspace_runner_exp.error());
    }

    return submission_processor(
        std::move(dependencies_value.judge_queue_port_value),
        std::move(dependencies_value.judge_submission_port_value),
        std::move(dependencies_value.execution_engine_value),
        std::move(dependencies_value.judge_evaluator_value),
        std::move(*workspace_runner_exp)
    );
}

submission_processor::submission_processor(
    judge_queue_port judge_queue_port_value,
    judge_submission_port judge_submission_port_value,
    execution_engine execution_engine_value,
    judge_evaluator judge_evaluator_value,
    workspace_runner workspace_runner_value
) :
    judge_queue_port_(std::move(judge_queue_port_value)),
    judge_submission_port_(std::move(judge_submission_port_value)),
    execution_engine_(std::move(execution_engine_value)),
    judge_evaluator_(std::move(judge_evaluator_value)),
    workspace_runner_(std::move(workspace_runner_value)){}

submission_processor::submission_processor(
    submission_processor&& other
) noexcept = default;

submission_processor& submission_processor::operator=(
    submission_processor&& other
) noexcept = default;

submission_processor::~submission_processor() = default;

std::expected<void, judge_error> submission_processor::process_next_submission(
    std::chrono::seconds lease_duration,
    std::chrono::milliseconds notification_wait_timeout
){
    auto queued_submission_opt_exp = judge_queue_port_.poll_next_submission(
        lease_duration,
        notification_wait_timeout
    );
    if(!queued_submission_opt_exp){
        return std::unexpected(queued_submission_opt_exp.error());
    }
    if(!queued_submission_opt_exp->has_value()){
        return {};
    }

    const submission_dto::queued_submission& queued_submission_value =
        queued_submission_opt_exp->value();
    const auto execute_submission_exp = execute_submission(queued_submission_value);

    if(!execute_submission_exp){
        const auto requeue_submission_exp = requeue_submission(
            queued_submission_value.submission_id,
            to_string(execute_submission_exp.error())
        );
        if(!requeue_submission_exp){
            return std::unexpected(requeue_submission_exp.error());
        }
    }

    return {};
}

std::expected<void, judge_error> submission_processor::execute_submission(
    const submission_dto::queued_submission& queued_submission_value
){
    const auto mark_judging_exp =
        judge_submission_port_.mark_judging(
            queued_submission_value.submission_id
        );
    if(!mark_judging_exp){
        return std::unexpected(mark_judging_exp.error());
    }

    auto judge_submission_exp = workspace_runner_.with_submission_workspace(
        queued_submission_value.submission_id,
        [&](const std::filesystem::path& workspace_path)
            -> std::expected<judge_submission_data::process_submission_data, judge_error> {
            return process_submission_in_workspace(
                queued_submission_value,
                workspace_path
            );
        }
    );
    if(!judge_submission_exp){
        return std::unexpected(judge_submission_exp.error());
    }

    const auto finalize_submission_exp = finalize_submission(
        queued_submission_value.submission_id,
        judge_submission_exp->judge_result_value,
        judge_submission_exp->execution_report_value
    );
    if(!finalize_submission_exp){
        return std::unexpected(finalize_submission_exp.error());
    }

    return {};
}

std::expected<judge_submission_data::process_submission_data, judge_error>
submission_processor::process_submission_in_workspace(
    const submission_dto::queued_submission& queued_submission_value,
    const std::filesystem::path& workspace_path
){
    auto build_result_exp = execution_engine_.build(
        queued_submission_value,
        workspace_path
    );
    if(!build_result_exp){
        return std::unexpected(build_result_exp.error());
    }

    auto build_result_value = std::move(*build_result_exp);
    if(auto* compile_failure_value =
           std::get_if<program_build::compile_failure>(&build_result_value)){
        auto process_submission_data_value =
            judge_submission_data::make_process_submission_data(
                judge_result::compile_error,
                make_compile_failed_execution_report(*compile_failure_value)
            );
        return process_submission_data_value;
    }

    const auto* build_artifact_value =
        std::get_if<program_build::build_artifact>(&build_result_value);
    if(build_artifact_value == nullptr){
        return std::unexpected(
            judge_error{
                judge_error_code::validation_error,
                "missing build artifact"
            }
        );
    }

    auto execution_result_exp = execution_engine_.run(
        queued_submission_value.problem_id,
        *build_artifact_value
    );
    if(!execution_result_exp){
        return std::unexpected(execution_result_exp.error());
    }

    auto execution_result_value = std::move(*execution_result_exp);
    const auto judge_result_exp = judge_evaluator_.evaluate(
        execution_result_value.testcase_snapshot_value,
        execution_result_value.execution_report_value
    );
    if(!judge_result_exp){
        return std::unexpected(judge_result_exp.error());
    }

    auto process_submission_data_value =
        judge_submission_data::make_process_submission_data(
            *judge_result_exp,
            std::move(execution_result_value.execution_report_value)
        );
    return process_submission_data_value;
}

std::expected<void, judge_error> submission_processor::finalize_submission(
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

    const auto finalize_submission_exp =
        judge_submission_port_.finalize_submission(finalize_request_value);
    if(!finalize_submission_exp){
        return std::unexpected(finalize_submission_exp.error());
    }

    return {};
}

std::expected<void, judge_error> submission_processor::requeue_submission(
    std::int64_t submission_id,
    std::string reason
){
    return judge_submission_port_.requeue_submission_immediately(
        submission_id,
        std::move(reason)
    );
}
