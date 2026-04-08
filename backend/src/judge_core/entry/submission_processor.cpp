#include "judge_core/entry/submission_processor.hpp"

#include <utility>

enum class build_outcome_for_submission{
    build_success,
    user_compile_error,
    compile_resource_exceeded,
    build_infra_failure
};

static build_outcome_for_submission classify_build_outcome_for_submission(
    const build_bundle& build_result_value
){
    if(build_result_value.success()){
        return build_outcome_for_submission::build_success;
    }

    if(build_result_value.is_user_compile_error()){
        return build_outcome_for_submission::user_compile_error;
    }

    if(build_result_value.is_compile_resource_exceeded()){
        return build_outcome_for_submission::compile_resource_exceeded;
    }

    return build_outcome_for_submission::build_infra_failure;
}

struct build_outcome_policy{
    static submission_decision make_user_compile_error_decision(
        const build_bundle::user_compile_error& user_compile_error_value
    ){
        return make_compile_error_decision(
            user_compile_error_value.compile_execution
        );
    }

    static submission_decision make_compile_resource_exceeded_decision(
        const build_bundle::compile_resource_exceeded& compile_resource_exceeded_value
    ){
        return make_compile_error_decision(
            compile_resource_exceeded_value.compile_execution
        );
    }

private:
    static submission_decision make_compile_error_decision(
        const execution_report::testcase_execution& compile_execution_value
    ){
        execution_report::batch execution_report_value;
        execution_report_value.compile_failed = true;
        execution_report_value.executions.push_back(compile_execution_value);

        submission_decision submission_decision_value;
        submission_decision_value.judge_result_value = judge_result::compile_error;
        submission_decision_value.execution_report_value =
            std::move(execution_report_value);
        return submission_decision_value;
    }
};

std::expected<submission_processor, judge_error> submission_processor::create(
    dependencies dependencies_value
){
    auto submission_lifecycle_exp = submission_lifecycle::create(
        std::move(dependencies_value.judge_submission_facade_value)
    );
    if(!submission_lifecycle_exp){
        return std::unexpected(submission_lifecycle_exp.error());
    }

    auto workspace_manager_exp = workspace_manager::create(
        std::move(dependencies_value.source_root_path)
    );
    if(!workspace_manager_exp){
        return std::unexpected(workspace_manager_exp.error());
    }

    return submission_processor(
        std::move(*submission_lifecycle_exp),
        std::move(dependencies_value.testcase_snapshot_facade_value),
        std::move(dependencies_value.submission_builder_value),
        std::move(dependencies_value.program_executor_value),
        std::move(dependencies_value.judge_evaluator_value),
        std::move(*workspace_manager_exp)
    );
}

submission_processor::submission_processor(
    submission_lifecycle submission_lifecycle_value,
    testcase_snapshot_facade testcase_snapshot_facade_value,
    submission_builder submission_builder_value,
    program_executor program_executor_value,
    judge_evaluator judge_evaluator_value,
    workspace_manager workspace_manager_value
) :
    submission_lifecycle_(std::move(submission_lifecycle_value)),
    testcase_snapshot_facade_(std::move(testcase_snapshot_facade_value)),
    submission_builder_(std::move(submission_builder_value)),
    program_executor_(std::move(program_executor_value)),
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
    const submission_dto::leased_submission& leased_submission_value
){
    const auto complete_with_failure =
        [&](judge_error error_value) -> std::expected<void, judge_error> {
        return submission_lifecycle_.complete(
            leased_submission_value,
            std::expected<submission_decision, judge_error>{
                std::unexpected(std::move(error_value))
            }
        );
    };

    const auto mark_judging_exp = submission_lifecycle_.mark_judging(
        leased_submission_value
    );
    if(!mark_judging_exp){
        return std::unexpected(mark_judging_exp.error());
    }

    auto workspace_session_exp = workspace_manager_.create(
        leased_submission_value.submission_id,
        leased_submission_value.attempt_no
    );
    if(!workspace_session_exp){
        return complete_with_failure(workspace_session_exp.error());
    }

    auto workspace_session_value = std::move(*workspace_session_exp);

    auto submission_decision_exp = [&]()
        -> std::expected<submission_decision, judge_error> {
        const auto source_file_path_exp =
            workspace_session_value.write_source_file(
                leased_submission_value.language,
                leased_submission_value.source_code
            );
        if(!source_file_path_exp){
            return std::unexpected(source_file_path_exp.error());
        }

        auto build_result_value = submission_builder_.build(
            *source_file_path_exp
        );
        switch(classify_build_outcome_for_submission(build_result_value)){
        case build_outcome_for_submission::build_success:
            break;
        case build_outcome_for_submission::user_compile_error:
            return build_outcome_policy::make_user_compile_error_decision(
                build_result_value.user_compile_error_value()
            );
        case build_outcome_for_submission::compile_resource_exceeded:
            return build_outcome_policy::make_compile_resource_exceeded_decision(
                build_result_value.compile_resource_exceeded_value()
            );
        case build_outcome_for_submission::build_infra_failure:
            return std::unexpected(build_result_value.infra_failure().error);
        }

        auto testcase_snapshot_exp = testcase_snapshot_facade_.acquire(
            leased_submission_value.problem_id,
            leased_submission_value.problem_version
        );
        if(!testcase_snapshot_exp){
            return std::unexpected(testcase_snapshot_exp.error());
        }

        auto execution_report_exp = program_executor_.run(
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
        return complete_with_failure(close_workspace_exp.error());
    }

    const auto finalize_submission_exp = submission_lifecycle_.complete(
        leased_submission_value,
        std::move(submission_decision_exp)
    );
    if(!finalize_submission_exp){
        return std::unexpected(finalize_submission_exp.error());
    }

    return {};
}
