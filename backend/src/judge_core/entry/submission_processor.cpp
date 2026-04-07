#include "judge_core/entry/submission_processor.hpp"

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

    auto workspace_manager_exp = workspace_manager::create(
        std::move(dependencies_value.source_root_path)
    );
    if(!workspace_manager_exp){
        return std::unexpected(workspace_manager_exp.error());
    }

    return submission_processor(
        std::move(*submission_lifecycle_exp),
        std::move(*snapshot_provider_exp),
        std::move(dependencies_value.submission_builder_value),
        std::move(dependencies_value.submission_executor_value),
        std::move(dependencies_value.judge_evaluator_value),
        std::move(*workspace_manager_exp)
    );
}

submission_processor::submission_processor(
    submission_lifecycle submission_lifecycle_value,
    snapshot_provider snapshot_provider_value,
    submission_builder submission_builder_value,
    submission_executor submission_executor_value,
    judge_evaluator judge_evaluator_value,
    workspace_manager workspace_manager_value
) :
    submission_lifecycle_(std::move(submission_lifecycle_value)),
    snapshot_provider_(std::move(snapshot_provider_value)),
    submission_builder_(std::move(submission_builder_value)),
    submission_executor_(std::move(submission_executor_value)),
    judge_evaluator_(std::move(judge_evaluator_value)),
    workspace_manager_(std::move(workspace_manager_value)){}

submission_processor::submission_processor(
    submission_processor&& other
) noexcept = default;

submission_processor& submission_processor::operator=(
    submission_processor&& other
) noexcept = default;

submission_processor::~submission_processor() = default;

std::expected<void, judge_error> submission_processor::process(
    const submission_dto::queued_submission& queued_submission_value
){
    const auto requeue_submission = [&](const judge_error& error_value){
        return submission_lifecycle_.requeue(
            queued_submission_value,
            error_value
        );
    };

    const auto mark_judging_exp = submission_lifecycle_.mark_judging(
        queued_submission_value
    );
    if(!mark_judging_exp){
        return requeue_submission(mark_judging_exp.error());
    }

    auto workspace_session_exp = workspace_manager_.create(
        queued_submission_value.submission_id
    );
    if(!workspace_session_exp){
        return requeue_submission(workspace_session_exp.error());
    }

    auto workspace_session_value = std::move(*workspace_session_exp);

    auto submission_decision_exp = [&]()
        -> std::expected<submission_decision, judge_error> {
        auto build_result_exp = submission_builder_.build(
            queued_submission_value,
            workspace_session_value
        );
        if(!build_result_exp){
            return std::unexpected(build_result_exp.error());
        }

        auto build_result_value = std::move(*build_result_exp);
        if(build_result_value.compile_failed()){
            return judge_evaluator_.evaluate_compile_failure(
                build_result_value.failure()
            );
        }

        auto testcase_snapshot_exp = snapshot_provider_.acquire(
            queued_submission_value.problem_id
        );
        if(!testcase_snapshot_exp){
            return std::unexpected(testcase_snapshot_exp.error());
        }

        auto execution_report_exp = submission_executor_.execute(
            build_result_value.artifact(),
            *testcase_snapshot_exp
        );
        if(!execution_report_exp){
            return std::unexpected(execution_report_exp.error());
        }

        return judge_evaluator_.evaluate_execution(
            *testcase_snapshot_exp,
            std::move(*execution_report_exp)
        );
    }();

    const auto close_workspace_exp = workspace_session_value.close();
    if(!close_workspace_exp){
        return requeue_submission(close_workspace_exp.error());
    }

    if(!submission_decision_exp){
        return requeue_submission(submission_decision_exp.error());
    }

    const auto finalize_submission_exp = submission_lifecycle_.finalize(
        queued_submission_value,
        *submission_decision_exp
    );
    if(!finalize_submission_exp){
        return requeue_submission(finalize_submission_exp.error());
    }

    return {};
}
