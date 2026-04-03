#pragma once

#include "error/sandbox_error.hpp"
#include "judge_core/infrastructure/execution_plan.hpp"
#include "judge_core/infrastructure/program_build_types.hpp"

#include <expected>

class launch_planner{
public:
    launch_planner(
        std::filesystem::path python_path,
        std::filesystem::path java_runtime_path
    );

    std::expected<program_launch::execution_plan, sandbox_error> make_execution_plan(
        const program_build::build_artifact& build_artifact_value
    );

private:
    std::filesystem::path python_path_;
    std::filesystem::path java_runtime_path_;
};
