#include "judge_core/application/execution_engine.hpp"

#include "judge_core/infrastructure/judge_workspace.hpp"
#include "judge_core/infrastructure/testcase_runner.hpp"

#include <utility>

namespace{
    judge_error make_invalid_build_artifact_error(){
        return judge_error{
            judge_error_code::validation_error,
            "build artifact is not runnable"
        };
    }
}

std::expected<execution_engine, judge_error> execution_engine::create(
    testcase_snapshot_port testcase_snapshot_port_value
){
    auto program_builder_exp = program_builder::create();
    if(!program_builder_exp){
        return std::unexpected(program_builder_exp.error());
    }

    auto launch_planner_exp = launch_planner::create();
    if(!launch_planner_exp){
        return std::unexpected(launch_planner_exp.error());
    }

    return execution_engine{
        std::move(*program_builder_exp),
        std::move(*launch_planner_exp),
        std::move(testcase_snapshot_port_value)
    };
}

execution_engine::execution_engine(
    program_builder program_builder_value,
    launch_planner launch_planner_value,
    testcase_snapshot_port testcase_snapshot_port_value
) :
    program_builder_(std::move(program_builder_value)),
    launch_planner_(std::move(launch_planner_value)),
    testcase_snapshot_port_(std::move(testcase_snapshot_port_value)){}

execution_engine::execution_engine(
    execution_engine&& other
) noexcept = default;

execution_engine& execution_engine::operator=(
    execution_engine&& other
) noexcept = default;

execution_engine::~execution_engine() = default;

std::expected<std::filesystem::path, judge_error>
execution_engine::write_submission_source(
    const submission_dto::queued_submission& queued_submission_value,
    const std::filesystem::path& workspace_path
){
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

std::expected<execution_engine::build_result, judge_error> execution_engine::build(
    const submission_dto::queued_submission& queued_submission_value,
    const std::filesystem::path& workspace_path
){
    const auto source_file_path_exp = write_submission_source(
        queued_submission_value,
        workspace_path
    );
    if(!source_file_path_exp){
        return std::unexpected(source_file_path_exp.error());
    }

    auto build_source_exp = program_builder_.build_source(*source_file_path_exp);
    if(!build_source_exp){
        return std::unexpected(judge_error{build_source_exp.error()});
    }

    auto build_artifact_value = std::move(*build_source_exp);
    if(!build_artifact_value.is_runnable()){
        return build_result{std::move(*build_artifact_value.compile_failure_opt_)};
    }

    return build_result{std::move(build_artifact_value)};
}

std::expected<execution_engine::execution_result, judge_error> execution_engine::run(
    std::int64_t problem_id,
    const program_build::build_artifact& build_artifact_value
){
    if(!build_artifact_value.is_runnable()){
        return std::unexpected(make_invalid_build_artifact_error());
    }

    const auto execution_plan_exp =
        launch_planner_.make_execution_plan(build_artifact_value);
    if(!execution_plan_exp){
        return std::unexpected(judge_error{execution_plan_exp.error()});
    }

    auto testcase_snapshot_exp = testcase_snapshot_port_.acquire(problem_id);
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

    execution_result execution_result_value;
    execution_result_value.testcase_snapshot_value = std::move(*testcase_snapshot_exp);
    execution_result_value.execution_report_value = std::move(*execution_report_exp);
    return execution_result_value;
}
