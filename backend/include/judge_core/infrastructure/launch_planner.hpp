#pragma once

#include "error/sandbox_error.hpp"
#include "judge_core/infrastructure/execution_plan.hpp"
#include "judge_core/infrastructure/program_build_types.hpp"

#include <expected>
#include <mutex>
#include <optional>

class launch_planner{
public:
    static launch_planner& instance();

    std::expected<program_launch::execution_plan, sandbox_error> make_execution_plan(
        const program_build::build_artifact& build_artifact_value
    );

private:
    launch_planner() = default;

    void initialize_if_needed();

    std::mutex initialize_mutex_;
    std::optional<std::filesystem::path> python_path_;
    std::optional<std::filesystem::path> java_runtime_path_;
};
