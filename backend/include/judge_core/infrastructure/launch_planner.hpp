#pragma once
		
#include "error/judge_error.hpp"
#include "error/sandbox_error.hpp"
#include "judge_core/infrastructure/execution_plan.hpp"
#include "judge_core/types/runnable_program.hpp"

#include <expected>

class launch_planner{
public:
    static std::expected<launch_planner, judge_error> create();

    launch_planner(
        std::filesystem::path python_path,
        std::filesystem::path java_runtime_path
    );

    std::expected<program_launch::execution_plan, sandbox_error> make_execution_plan(
        const runnable_program& runnable_program_value
    );

private:
    std::filesystem::path python_path_;
    std::filesystem::path java_runtime_path_;
};
