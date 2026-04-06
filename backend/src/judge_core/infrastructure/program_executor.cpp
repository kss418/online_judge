#include "judge_core/infrastructure/program_executor.hpp"

#include "judge_core/infrastructure/testcase_runner.hpp"

#include <utility>

std::expected<program_executor, judge_error> program_executor::create(){
    auto launch_planner_exp = launch_planner::create();
    if(!launch_planner_exp){
        return std::unexpected(launch_planner_exp.error());
    }

    return program_executor{std::move(*launch_planner_exp)};
}

program_executor::program_executor(launch_planner launch_planner_value) :
    launch_planner_(std::move(launch_planner_value)){}

std::expected<execution_report::batch, judge_error> program_executor::run(
    const runnable_program& runnable_program_value,
    const testcase_snapshot& testcase_snapshot_value
){
    const auto execution_plan_exp =
        launch_planner_.make_execution_plan(runnable_program_value);
    if(!execution_plan_exp){
        return std::unexpected(judge_error{execution_plan_exp.error()});
    }

    auto execution_report_exp = testcase_runner::run_all_testcases(
        *execution_plan_exp,
        testcase_snapshot_value
    );
    if(!execution_report_exp){
        return std::unexpected(execution_report_exp.error());
    }

    return *execution_report_exp;
}
