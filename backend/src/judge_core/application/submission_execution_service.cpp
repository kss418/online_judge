#include "judge_core/application/submission_execution_service.hpp"

#include "common/timer.hpp"
#include "judge_core/infrastructure/judge_workspace.hpp"
#include "judge_core/infrastructure/launch_planner.hpp"
#include "judge_core/infrastructure/program_builder.hpp"
#include "judge_core/infrastructure/testcase_runner.hpp"
#include "judge_core/policy/judge_expectation_loader.hpp"
#include "judge_core/policy/judge_policy.hpp"

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
        const program_build::compile_failure& compile_failure_value,
        std::int64_t prepare_elapsed_ms
    ){
        execution_report::batch execution_report_value;
        execution_report_value.compile_failed = true;
        execution_report_value.prepare_elapsed_ms = prepare_elapsed_ms;
        execution_report_value.executions.push_back(
            make_testcase_execution(compile_failure_value)
        );
        return execution_report_value;
    }
}

submission_execution_service::submission_execution_service(
    testcase_snapshot_service testcase_snapshot_service
) :
    testcase_snapshot_service_(std::move(testcase_snapshot_service)){}

std::expected<std::filesystem::path, judge_error>
submission_execution_service::prepare_workspace(
    const submission_dto::queued_submission& queued_submission_value,
    const std::filesystem::path& workspace_path
){
    const auto reset_workspace_exp = judge_workspace::reset(workspace_path);
    if(!reset_workspace_exp){
        return std::unexpected(judge_error{reset_workspace_exp.error()});
    }

    const auto source_file_path_exp = judge_workspace::write_source_file(
        workspace_path,
        queued_submission_value.language,
        queued_submission_value.source_code
    );
    if(!source_file_path_exp){
        return std::unexpected(judge_error{source_file_path_exp.error()});
    }

    return *source_file_path_exp;
}

std::expected<judge_submission_data::process_submission_data, judge_error>
submission_execution_service::process_submission(
    const submission_dto::queued_submission& queued_submission_value,
    const std::filesystem::path& workspace_path,
    db_connection& testcase_snapshot_connection
){
    std::int64_t prepare_workspace_elapsed_ms = 0;
    std::int64_t build_source_elapsed_ms = 0;
    std::int64_t make_execution_plan_elapsed_ms = 0;
    std::int64_t testcase_snapshot_elapsed_ms = 0;

    const auto source_file_path_exp = timer::measure_elapsed_ms(
        prepare_workspace_elapsed_ms,
        [this, &queued_submission_value, &workspace_path]{
            return prepare_workspace(queued_submission_value, workspace_path);
        }
    );
    if(!source_file_path_exp){
        return std::unexpected(source_file_path_exp.error());
    }

    const auto build_source_exp = timer::measure_elapsed_ms(
        build_source_elapsed_ms,
        [&source_file_path_exp]{
            return program_builder::instance().build_source(*source_file_path_exp);
        }
    );
    if(!build_source_exp){
        return std::unexpected(judge_error{build_source_exp.error()});
    }

    if(!build_source_exp->is_runnable()){
        auto process_submission_data_value =
            judge_submission_data::make_process_submission_data(
                judge_result::compile_error,
                make_compile_failed_execution_report(
                    *build_source_exp->compile_failure_opt_,
                    build_source_elapsed_ms
                )
            );
        process_submission_data_value.prepare_workspace_elapsed_ms =
            prepare_workspace_elapsed_ms;
        return process_submission_data_value;
    }

    const auto execution_plan_exp = timer::measure_elapsed_ms(
        make_execution_plan_elapsed_ms,
        [&build_source_exp]{
            return launch_planner::instance().make_execution_plan(*build_source_exp);
        }
    );
    if(!execution_plan_exp){
        return std::unexpected(judge_error{execution_plan_exp.error()});
    }

    const auto testcase_snapshot_exp = timer::measure_elapsed_ms(
        testcase_snapshot_elapsed_ms,
        [this, &queued_submission_value, &testcase_snapshot_connection]{
            return testcase_snapshot_service_.acquire(
                testcase_snapshot_connection,
                queued_submission_value.problem_id
            );
        }
    );
    if(!testcase_snapshot_exp){
        return std::unexpected(testcase_snapshot_exp.error());
    }

    auto execution_report_exp = testcase_runner::run_all_testcases(
        *execution_plan_exp,
        *testcase_snapshot_exp
    );
    if(!execution_report_exp){
        return std::unexpected(execution_report_exp.error());
    }
    execution_report_exp->prepare_elapsed_ms =
        build_source_elapsed_ms + make_execution_plan_elapsed_ms;

    const auto judge_expectation_exp = judge_expectation_loader::load(
        *testcase_snapshot_exp
    );
    if(!judge_expectation_exp){
        return std::unexpected(judge_error{judge_expectation_exp.error()});
    }

    const auto judge_result_exp = judge_policy::check_result(
        *judge_expectation_exp,
        *execution_report_exp
    );
    if(!judge_result_exp){
        return std::unexpected(judge_result_exp.error());
    }

    auto process_submission_data_value =
        judge_submission_data::make_process_submission_data(
            *judge_result_exp,
            std::move(*execution_report_exp)
        );
    process_submission_data_value.prepare_workspace_elapsed_ms =
        prepare_workspace_elapsed_ms;
    process_submission_data_value.testcase_snapshot_elapsed_ms =
        testcase_snapshot_elapsed_ms;
    return process_submission_data_value;
}
