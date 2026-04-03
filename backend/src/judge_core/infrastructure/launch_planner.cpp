#include "judge_core/infrastructure/launch_planner.hpp"

#include "judge_core/infrastructure/program_handler.hpp"

launch_planner::launch_planner(
    std::shared_ptr<const program_handler_registry> handler_registry
) :
    handler_registry_(std::move(handler_registry))
{}

std::expected<program_launch::execution_plan, sandbox_error> launch_planner::make_execution_plan(
    const program_build::build_artifact& build_artifact_value
){
    if(handler_registry_ == nullptr){
        return std::unexpected(sandbox_error::invalid_argument);
    }

    const auto handler_exp = handler_registry_->find_for_build_artifact(
        build_artifact_value
    );
    if(!handler_exp){
        return std::unexpected(handler_exp.error());
    }

    return (*handler_exp)->make_execution_plan(build_artifact_value);
}
