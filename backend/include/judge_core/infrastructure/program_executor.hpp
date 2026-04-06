#pragma once

#include "error/judge_error.hpp"
#include "judge_core/infrastructure/launch_planner.hpp"
#include "judge_core/types/execution_report.hpp"
#include "judge_core/types/runnable_program.hpp"
#include "judge_core/types/testcase_snapshot.hpp"

#include <expected>

class program_executor{
public:
    static std::expected<program_executor, judge_error> create();

    explicit program_executor(launch_planner launch_planner_value);

    std::expected<execution_report::batch, judge_error> run(
        const runnable_program& runnable_program_value,
        const testcase_snapshot& testcase_snapshot_value
    );

private:
    launch_planner launch_planner_;
};
