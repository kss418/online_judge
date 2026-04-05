#include "judge_core/application/execution_engine.hpp"

#include "judge_core/infrastructure/judge_workspace.hpp"
#include "judge_core/infrastructure/program_build_types.hpp"
#include "judge_core/infrastructure/testcase_runner.hpp"

#include <utility>

namespace{
    runnable_program_language to_runnable_program_language(
        program_build::source_language language_value
    ){
        switch(language_value){
        case program_build::source_language::cpp:
            return runnable_program_language::cpp;
        case program_build::source_language::python:
            return runnable_program_language::python;
        case program_build::source_language::java:
            return runnable_program_language::java;
        }

        return runnable_program_language::cpp;
    }

    compile_failure make_compile_failure(
        program_build::compile_failure compile_failure_value
    ){
        compile_failure app_compile_failure_value;
        app_compile_failure_value.exit_code = compile_failure_value.exit_code_;
        app_compile_failure_value.stderr_text =
            std::move(compile_failure_value.stderr_text_);
        return app_compile_failure_value;
    }

    runnable_program make_runnable_program(
        program_build::build_artifact build_artifact_value
    ){
        runnable_program runnable_program_value;
        runnable_program_value.language =
            to_runnable_program_language(build_artifact_value.language_);
        runnable_program_value.workspace_host_path =
            std::move(build_artifact_value.workspace_host_path_);
        runnable_program_value.entry_file_host_path =
            std::move(build_artifact_value.entry_file_host_path_);
        runnable_program_value.main_class_name =
            std::move(build_artifact_value.main_class_name_);
        return runnable_program_value;
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
        return build_result{
            make_compile_failure(std::move(*build_artifact_value.compile_failure_opt_))
        };
    }

    return build_result{make_runnable_program(std::move(build_artifact_value))};
}

std::expected<execution_engine::execution_result, judge_error> execution_engine::run(
    std::int64_t problem_id,
    const runnable_program& runnable_program_value
){
    const auto execution_plan_exp =
        launch_planner_.make_execution_plan(runnable_program_value);
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
