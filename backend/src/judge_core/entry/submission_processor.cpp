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
        std::move(dependencies_value.judge_queue_facade_value),
        std::move(*submission_lifecycle_exp),
        std::move(*snapshot_provider_exp),
        std::move(dependencies_value.submission_builder_value),
        std::move(dependencies_value.submission_executor_value),
        std::move(dependencies_value.judge_evaluator_value),
        std::move(*workspace_manager_exp)
    );
}

submission_processor::submission_processor(
    judge_queue_facade judge_queue_facade_value,
    submission_lifecycle submission_lifecycle_value,
    snapshot_provider snapshot_provider_value,
    submission_builder submission_builder_value,
    submission_executor submission_executor_value,
    judge_evaluator judge_evaluator_value,
    workspace_manager workspace_manager_value
) :
    judge_queue_facade_(std::move(judge_queue_facade_value)),
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

    std::optional<judge_error> processing_error_opt = std::nullopt;
    std::optional<submission_decision> submission_decision_opt = std::nullopt;

    auto build_result_exp = submission_builder_.build(
        submission_builder::build_input{
            .queued_submission_value = queued_submission_value,
            .workspace_session_value = workspace_session_value,
        }
    );
    if(!build_result_exp){
        processing_error_opt = build_result_exp.error();
    }
    else{
        auto build_result_value = std::move(*build_result_exp);
        execution_bundle execution_bundle_value = execution_bundle::skipped();
        std::optional<testcase_snapshot> testcase_snapshot_opt = std::nullopt;

        if(build_result_value.success()){
            auto testcase_snapshot_exp = snapshot_provider_.acquire(
                queued_submission_value.problem_id
            );
            if(!testcase_snapshot_exp){
                processing_error_opt = testcase_snapshot_exp.error();
            }
            else{
                testcase_snapshot_opt = std::move(*testcase_snapshot_exp);

                auto execution_bundle_exp = submission_executor_.execute(
                    submission_executor::execution_input{
                        .runnable_program_value = build_result_value.artifact(),
                        .testcase_snapshot_value = *testcase_snapshot_opt,
                        .workspace_session_value = workspace_session_value,
                    }
                );
                if(!execution_bundle_exp){
                    processing_error_opt = execution_bundle_exp.error();
                }
                else{
                    execution_bundle_value = std::move(*execution_bundle_exp);
                }
            }
        }

        if(!processing_error_opt){
            auto submission_decision_exp = judge_evaluator_.evaluate(
                judge_evaluator::evaluation_input{
                    .queued_submission_value = queued_submission_value,
                    .build_bundle_value = build_result_value,
                    .execution_bundle_value = execution_bundle_value,
                    .testcase_snapshot_value_ptr =
                        testcase_snapshot_opt ? &*testcase_snapshot_opt : nullptr,
                }
            );
            if(!submission_decision_exp){
                processing_error_opt = submission_decision_exp.error();
            }
            else{
                submission_decision_opt = std::move(*submission_decision_exp);
            }
        }
    }

    const auto close_workspace_exp = workspace_session_value.close();
    if(!close_workspace_exp){
        return requeue_submission(close_workspace_exp.error());
    }

    if(processing_error_opt){
        return requeue_submission(*processing_error_opt);
    }

    if(!submission_decision_opt.has_value()){
        return requeue_submission(
            judge_error{
                judge_error_code::validation_error,
                "missing submission decision"
            }
        );
    }

    const auto finalize_submission_exp = submission_lifecycle_.finalize(
        queued_submission_value,
        *submission_decision_opt
    );
    if(!finalize_submission_exp){
        return requeue_submission(finalize_submission_exp.error());
    }

    return {};
}
