#include "judge_core/infrastructure/launch_planner.hpp"

#include "common/env_util.hpp"
#include "judge_core/infrastructure/judge_workspace.hpp"

namespace{
    std::expected<std::filesystem::path, judge_error> load_required_path_env(
        const char* key,
        std::string empty_value_message
    ){
        const auto path_text_exp = env_util::require_env(key);
        if(!path_text_exp){
            return std::unexpected(judge_error{path_text_exp.error()});
        }

        const std::filesystem::path path_value(*path_text_exp);
        if(path_value.empty()){
            return std::unexpected(
                judge_error{
                    judge_error_code::validation_error,
                    std::move(empty_value_message)
                }
            );
        }

        return path_value;
    }
}

std::expected<launch_planner, judge_error> launch_planner::create(){
    const auto python_path_exp = load_required_path_env(
        "JUDGE_PYTHON_PATH",
        "python path is not configured"
    );
    if(!python_path_exp){
        return std::unexpected(python_path_exp.error());
    }

    const auto java_runtime_path_exp = load_required_path_env(
        "JUDGE_JAVA_RUNTIME_PATH",
        "java runtime path is not configured"
    );
    if(!java_runtime_path_exp){
        return std::unexpected(java_runtime_path_exp.error());
    }

    return launch_planner{
        std::move(*python_path_exp),
        std::move(*java_runtime_path_exp)
    };
}

launch_planner::launch_planner(
    std::filesystem::path python_path,
    std::filesystem::path java_runtime_path
) :
    python_path_(std::move(python_path)),
    java_runtime_path_(std::move(java_runtime_path))
{}

std::expected<program_launch::execution_plan, sandbox_error> launch_planner::make_execution_plan(
    const program_build::build_artifact& build_artifact_value
){
    if(!build_artifact_value.is_runnable()){
        return std::unexpected(sandbox_error::invalid_argument);
    }

    program_launch::execution_plan execution_plan_value;

    switch(build_artifact_value.language_){
    case program_build::source_language::cpp: {
        if(
            build_artifact_value.workspace_host_path_.empty() ||
            build_artifact_value.entry_file_host_path_.empty()
        ){
            return std::unexpected(sandbox_error::invalid_argument);
        }

        const auto binary_sandbox_path = judge_workspace::make_sandbox_path(
            build_artifact_value.workspace_host_path_,
            build_artifact_value.entry_file_host_path_
        );
        if(binary_sandbox_path.empty()){
            return std::unexpected(sandbox_error::invalid_argument);
        }

        execution_plan_value.workspace_host_path_ =
            build_artifact_value.workspace_host_path_;
        execution_plan_value.run_command_args_.push_back(binary_sandbox_path.string());
        return execution_plan_value;
    }

    case program_build::source_language::python: {
        if(
            build_artifact_value.workspace_host_path_.empty() ||
            build_artifact_value.entry_file_host_path_.empty() ||
            python_path_.empty()
        ){
            return std::unexpected(sandbox_error::invalid_argument);
        }

        const auto source_sandbox_path = judge_workspace::make_sandbox_path(
            build_artifact_value.workspace_host_path_,
            build_artifact_value.entry_file_host_path_
        );
        if(source_sandbox_path.empty()){
            return std::unexpected(sandbox_error::invalid_argument);
        }

        execution_plan_value.workspace_host_path_ =
            build_artifact_value.workspace_host_path_;
        execution_plan_value.run_command_args_.push_back(python_path_.string());
        execution_plan_value.run_command_args_.push_back(source_sandbox_path.string());
        return execution_plan_value;
    }

    case program_build::source_language::java: {
        if(
            build_artifact_value.workspace_host_path_.empty() ||
            build_artifact_value.main_class_name_.empty() ||
            java_runtime_path_.empty()
        ){
            return std::unexpected(sandbox_error::invalid_argument);
        }

        execution_plan_value.workspace_host_path_ =
            build_artifact_value.workspace_host_path_;
        execution_plan_value.mount_profile_ =
            sandbox_runner::mount_profile::java_profile;
        execution_plan_value.run_command_args_.push_back(java_runtime_path_.string());
        execution_plan_value.run_command_args_.push_back("-XX:-UsePerfData");
        execution_plan_value.run_command_args_.push_back("-cp");
        execution_plan_value.run_command_args_.push_back(
            judge_workspace::sandbox_workspace_path().string()
        );
        execution_plan_value.run_command_args_.push_back(
            build_artifact_value.main_class_name_
        );
        return execution_plan_value;
    }
    }

    return std::unexpected(sandbox_error::invalid_argument);
}
