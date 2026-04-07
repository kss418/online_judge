#include "judge_core/application/compile_failure_report_mapper.hpp"
#include "judge_core/application/execution_bundle.hpp"
#include "judge_core/entry/submission_processor.hpp"

#include <optional>
#include <utility>

std::expected<submission_processor, judge_error> submission_processor::create(
    dependencies dependencies_value
){
    auto submission_lifecycle_exp = submission_lifecycle::create(
        std::move(dependencies_value.judge_submission_facade_value)
    );
    if(!submission_lifecycle_exp){
        return std::unexpected(submission_lifecycle_exp.error());
    }

    auto snapshot_provider_exp = snapshot_provider::create(
        std::move(dependencies_value.testcase_snapshot_facade_value)
    );
    if(!snapshot_provider_exp){
        return std::unexpected(snapshot_provider_exp.error());
    }

    auto workspace_runner_exp = workspace_runner::create(
        std::move(dependencies_value.source_root_path)
    );
    if(!workspace_runner_exp){
        return std::unexpected(workspace_runner_exp.error());
    }

    return submission_processor(
        std::move(dependencies_value.judge_queue_facade_value),
        std::move(*submission_lifecycle_exp),
        std::move(*snapshot_provider_exp),
        std::move(dependencies_value.submission_builder_value),
        std::move(dependencies_value.submission_executor_value),
        std::move(dependencies_value.judge_evaluator_value),
        std::move(*workspace_runner_exp)
    );
}

submission_processor::submission_processor(
    judge_queue_facade judge_queue_facade_value,
    submission_lifecycle submission_lifecycle_value,
    snapshot_provider snapshot_provider_value,
    submission_builder submission_builder_value,
    submission_executor submission_executor_value,
    judge_evaluator judge_evaluator_value,
    workspace_runner workspace_runner_value
) :
    judge_queue_facade_(std::move(judge_queue_facade_value)),
    submission_lifecycle_(std::move(submission_lifecycle_value)),
    snapshot_provider_(std::move(snapshot_provider_value)),
    submission_builder_(std::move(submission_builder_value)),
    submission_executor_(std::move(submission_executor_value)),
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
    auto queued_submission_opt_exp = judge_queue_facade_.poll_next_submission(
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
        const auto requeue_submission_exp = submission_lifecycle_.requeue_submission(
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
    const auto mark_judging_exp = submission_lifecycle_.mark_judging(
        queued_submission_value.submission_id
    );
    if(!mark_judging_exp){
        return std::unexpected(mark_judging_exp.error());
    }

    auto judge_submission_exp = workspace_runner_.with_submission_workspace(
        queued_submission_value.submission_id,
        [&]()
            -> std::expected<judge_submission_data::process_submission_data, judge_error> {
            return process_submission_in_workspace(queued_submission_value);
        }
    );
    if(!judge_submission_exp){
        return std::unexpected(judge_submission_exp.error());
    }

    const auto finalize_submission_exp = submission_lifecycle_.finalize_submission(
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
    const submission_dto::queued_submission& queued_submission_value
){
    const auto source_file_path_exp = workspace_runner_.write_source_file(
        queued_submission_value.submission_id,
        queued_submission_value.language,
        queued_submission_value.source_code
    );
    if(!source_file_path_exp){
        return std::unexpected(source_file_path_exp.error());
    }

    auto build_result_exp = submission_builder_.build(
        *source_file_path_exp
    );
    if(!build_result_exp){
        return std::unexpected(build_result_exp.error());
    }

    auto build_result_value = std::move(*build_result_exp);
    execution_bundle execution_bundle_value = execution_bundle::skipped();
    std::optional<testcase_snapshot> testcase_snapshot_opt = std::nullopt;
    if(build_result_value.success()){
        const auto* runnable_program_value = build_result_value.runnable_program_opt();
        if(runnable_program_value == nullptr){
            return std::unexpected(
                judge_error{
                    judge_error_code::validation_error,
                    "missing runnable program"
                }
            );
        }

        auto testcase_snapshot_exp = snapshot_provider_.acquire(
            queued_submission_value.problem_id
        );
        if(!testcase_snapshot_exp){
            return std::unexpected(testcase_snapshot_exp.error());
        }
        testcase_snapshot_opt = std::move(*testcase_snapshot_exp);

        auto execution_report_exp = submission_executor_.execute(
            *runnable_program_value,
            *testcase_snapshot_opt
        );
        if(!execution_report_exp){
            return std::unexpected(execution_report_exp.error());
        }

        execution_bundle_value = execution_bundle::executed(
            std::move(*execution_report_exp)
        );
    }

    const auto judge_result_exp = judge_evaluator_.evaluate(
        judge_evaluator::evaluation_input{
            .build_bundle_value = build_result_value,
            .execution_bundle_value = execution_bundle_value,
            .testcase_snapshot_value_ptr =
                testcase_snapshot_opt ? &*testcase_snapshot_opt : nullptr,
        }
    );
    if(!judge_result_exp){
        return std::unexpected(judge_result_exp.error());
    }

    execution_report::batch execution_report_value;
    if(const auto* compile_failure_value =
           build_result_value.compile_failure_opt()){
        execution_report_value =
            compile_failure_report_mapper::make_execution_report(
                *compile_failure_value
            );
    }
    else{
        auto* executed_report_value = execution_bundle_value.execution_report_opt();
        if(executed_report_value == nullptr){
            return std::unexpected(
                judge_error{
                    judge_error_code::validation_error,
                    "missing execution report"
                }
            );
        }
        execution_report_value = std::move(*executed_report_value);
    }

    auto process_submission_data_value =
        judge_submission_data::make_process_submission_data(
            *judge_result_exp,
            std::move(execution_report_value)
        );
    return process_submission_data_value;
}
