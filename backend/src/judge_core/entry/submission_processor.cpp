#include "judge_core/entry/submission_processor.hpp"

#include "common/logger.hpp"

#include <utility>

namespace{
    execution_policy select_execution_policy(
        const submission_domain_dto::leased_submission& leased_submission_value,
        const testcase_snapshot& testcase_snapshot_value
    ){
        (void)leased_submission_value;
        (void)testcase_snapshot_value;
        return execution_policy{};
    }

    void log_workspace_cleanup_failure(
        const submission_domain_dto::leased_submission& leased_submission_value,
        const std::filesystem::path& workspace_path,
        bool finalize_succeeded,
        const judge_error& error_value
    ){
        logger::cerr()
            .log("workspace_cleanup_failed")
            .field("submission_id", leased_submission_value.submission_id)
            .field("attempt_no", leased_submission_value.attempt_no)
            .field("workspace_path", workspace_path.string())
            .field("finalize_succeeded", finalize_succeeded)
            .field("error_code", to_string(error_value.code))
            .field("error_message", error_value.message);
    }

    void close_workspace_best_effort(
        const submission_domain_dto::leased_submission& leased_submission_value,
        workspace_session& workspace_session_value,
        const std::filesystem::path& workspace_path,
        bool finalize_succeeded
    ){
        const auto close_workspace_exp = workspace_session_value.close();
        if(!close_workspace_exp){
            log_workspace_cleanup_failure(
                leased_submission_value,
                workspace_path,
                finalize_succeeded,
                close_workspace_exp.error()
            );
        }
    }
}

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
    const submission_domain_dto::leased_submission& leased_submission_value
){
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
        return submission_lifecycle_.apply_completion(
            leased_submission_value,
            submission_lifecycle_.make_infra_failure_completion(
                workspace_session_exp.error()
            )
        );
    }

    auto workspace_session_value = std::move(*workspace_session_exp);
    const auto workspace_path = workspace_session_value.path();

    auto submission_completion_value = [&]()
        -> submission_lifecycle::submission_completion {
        const auto source_file_path_exp =
            workspace_session_value.write_source_file(
                leased_submission_value.language,
                leased_submission_value.source_code
            );
        if(!source_file_path_exp){
            return submission_lifecycle_.make_infra_failure_completion(
                source_file_path_exp.error()
            );
        }

        auto build_result_value = submission_builder_.build(
            *source_file_path_exp
        );
        auto build_completion_opt = submission_lifecycle_.apply_build_policy(
            leased_submission_value,
            build_result_value
        );
        if(build_completion_opt.has_value()){
            return std::move(*build_completion_opt);
        }

        auto testcase_snapshot_exp = testcase_snapshot_facade_.acquire(
            leased_submission_value.problem_id,
            leased_submission_value.problem_version
        );
        if(!testcase_snapshot_exp){
            return submission_lifecycle_.make_infra_failure_completion(
                testcase_snapshot_exp.error()
            );
        }

        const execution_policy execution_policy_value =
            select_execution_policy(
                leased_submission_value,
                *testcase_snapshot_exp
            );

        auto execution_report_exp = program_executor_.run(
            build_result_value.artifact(),
            *testcase_snapshot_exp,
            execution_policy_value
        );
        if(!execution_report_exp){
            return submission_lifecycle_.make_infra_failure_completion(
                execution_report_exp.error()
            );
        }

        auto submission_decision_exp = judge_evaluator_.evaluate_execution(
            *testcase_snapshot_exp,
            std::move(*execution_report_exp)
        );
        if(!submission_decision_exp){
            return submission_lifecycle_.make_infra_failure_completion(
                submission_decision_exp.error()
            );
        }

        return submission_lifecycle_.make_decision_completion(
            leased_submission_value,
            *submission_decision_exp
        );
    }();

    const auto finalize_submission_exp = submission_lifecycle_.apply_completion(
        leased_submission_value,
        submission_completion_value
    );
    const bool finalize_succeeded = finalize_submission_exp.has_value();
    close_workspace_best_effort(
        leased_submission_value,
        workspace_session_value,
        workspace_path,
        finalize_succeeded
    );
    if(!finalize_submission_exp){
        return std::unexpected(finalize_submission_exp.error());
    }

    return {};
}
