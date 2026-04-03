#pragma once

#include "error/sandbox_error.hpp"
#include "judge_core/infrastructure/execution_plan.hpp"
#include "judge_core/infrastructure/program_build_types.hpp"

#include <expected>
#include <memory>

class program_handler_registry;

class launch_planner{
public:
    explicit launch_planner(
        std::shared_ptr<const program_handler_registry> handler_registry
    );

    std::expected<program_launch::execution_plan, sandbox_error> make_execution_plan(
        const program_build::build_artifact& build_artifact_value
    );

private:
    std::shared_ptr<const program_handler_registry> handler_registry_;
};
