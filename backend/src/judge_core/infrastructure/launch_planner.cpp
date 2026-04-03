#include "judge_core/infrastructure/launch_planner.hpp"

#include "judge_core/infrastructure/judge_workspace.hpp"

#include <cstdlib>

launch_planner& launch_planner::instance(){
    static launch_planner launch_planner_value;
    launch_planner_value.initialize_if_needed();
    return launch_planner_value;
}

void launch_planner::initialize_if_needed(){
    std::scoped_lock lock(initialize_mutex_);
    if(
        python_path_.has_value() &&
        java_runtime_path_.has_value()
    ){
        return;
    }

    const char* python_path = std::getenv("JUDGE_PYTHON_PATH");
    if(python_path != nullptr && *python_path != '\0'){
        python_path_ = std::filesystem::path(python_path);
    }

    const char* java_runtime_path = std::getenv("JUDGE_JAVA_RUNTIME_PATH");
    if(java_runtime_path != nullptr && *java_runtime_path != '\0'){
        java_runtime_path_ = std::filesystem::path(java_runtime_path);
    }
}

std::expected<program_launch::execution_plan, sandbox_error> launch_planner::make_execution_plan(
    const program_build::build_artifact& build_artifact_value
){
    if(!build_artifact_value.is_runnable()){
        return std::unexpected(sandbox_error::invalid_argument);
    }

    program_launch::execution_plan execution_plan_value;

    switch(build_artifact_value.language_){
    case program_build::source_language::cpp: {
        if(build_artifact_value.entry_host_path_.empty()){
            return std::unexpected(sandbox_error::invalid_argument);
        }

        const auto workspace_host_path = build_artifact_value.entry_host_path_.parent_path();
        const auto binary_sandbox_path = judge_workspace::make_sandbox_path(
            workspace_host_path,
            build_artifact_value.entry_host_path_
        );
        if(binary_sandbox_path.empty()){
            return std::unexpected(sandbox_error::invalid_argument);
        }

        execution_plan_value.workspace_host_path_ = std::move(workspace_host_path);
        execution_plan_value.run_command_args_.push_back(binary_sandbox_path.string());
        return execution_plan_value;
    }

    case program_build::source_language::python: {
        if(
            build_artifact_value.entry_host_path_.empty() ||
            !python_path_.has_value() ||
            python_path_->empty()
        ){
            return std::unexpected(sandbox_error::invalid_argument);
        }

        const auto workspace_host_path = build_artifact_value.entry_host_path_.parent_path();
        const auto source_sandbox_path = judge_workspace::make_sandbox_path(
            workspace_host_path,
            build_artifact_value.entry_host_path_
        );
        if(source_sandbox_path.empty()){
            return std::unexpected(sandbox_error::invalid_argument);
        }

        execution_plan_value.workspace_host_path_ = std::move(workspace_host_path);
        execution_plan_value.run_command_args_.push_back(python_path_->string());
        execution_plan_value.run_command_args_.push_back(source_sandbox_path.string());
        return execution_plan_value;
    }

    case program_build::source_language::java: {
        if(
            build_artifact_value.entry_host_path_.empty() ||
            build_artifact_value.entry_name_.empty() ||
            !java_runtime_path_.has_value() ||
            java_runtime_path_->empty()
        ){
            return std::unexpected(sandbox_error::invalid_argument);
        }

        execution_plan_value.workspace_host_path_ = build_artifact_value.entry_host_path_;
        execution_plan_value.mount_profile_ = sandbox_runner::mount_profile::java_profile;
        execution_plan_value.run_command_args_.push_back(java_runtime_path_->string());
        execution_plan_value.run_command_args_.push_back("-XX:-UsePerfData");
        execution_plan_value.run_command_args_.push_back("-cp");
        execution_plan_value.run_command_args_.push_back(
            judge_workspace::sandbox_workspace_path().string()
        );
        execution_plan_value.run_command_args_.push_back(build_artifact_value.entry_name_);
        return execution_plan_value;
    }
    }

    return std::unexpected(sandbox_error::invalid_argument);
}
