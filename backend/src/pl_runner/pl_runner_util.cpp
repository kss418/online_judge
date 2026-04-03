#include "pl_runner/pl_runner_util.hpp"

#include "judge_core/judge_workspace.hpp"
#include "pl_runner/cpp_runner.hpp"
#include "pl_runner/java_runner.hpp"
#include "pl_runner/python_runner.hpp"

#include <cstdlib>
#include <utility>

namespace{
    constexpr std::chrono::milliseconds COMPILE_TIME_LIMIT{30000};
    constexpr std::int64_t COMPILE_MEMORY_LIMIT_MB = 1024;
}

pl_runner_util& pl_runner_util::instance(){
    static pl_runner_util pl_runner_util_value;
    pl_runner_util_value.initialize_if_needed();
    return pl_runner_util_value;
}

void pl_runner_util::initialize_if_needed(){
    std::scoped_lock lock(initialize_mutex_);
    if(
        cpp_compiler_path_.has_value() &&
        python_path_.has_value() &&
        java_compiler_path_.has_value() &&
        java_runtime_path_.has_value()
    ){
        return;
    }

    const char* cpp_compiler_path = std::getenv("JUDGE_CPP_COMPILER_PATH");
    if(cpp_compiler_path != nullptr && *cpp_compiler_path != '\0'){
        cpp_compiler_path_ = std::filesystem::path(cpp_compiler_path);
    }

    const char* python_path = std::getenv("JUDGE_PYTHON_PATH");
    if(python_path != nullptr && *python_path != '\0'){
        python_path_ = std::filesystem::path(python_path);
    }

    const char* java_compiler_path = std::getenv("JUDGE_JAVA_COMPILER_PATH");
    if(java_compiler_path != nullptr && *java_compiler_path != '\0'){
        java_compiler_path_ = std::filesystem::path(java_compiler_path);
    }

    const char* java_runtime_path = std::getenv("JUDGE_JAVA_RUNTIME_PATH");
    if(java_runtime_path != nullptr && *java_runtime_path != '\0'){
        java_runtime_path_ = std::filesystem::path(java_runtime_path);
    }
}

pl_runner_util::build_artifact pl_runner_util::make_compile_failed_artifact(
    source_language language,
    int exit_code,
    std::string stderr_text
){
    build_artifact build_artifact_value;
    build_artifact_value.language_ = language;
    sandbox_runner::run_result run_result_value;
    run_result_value.exit_code_ = exit_code;
    run_result_value.stderr_text_ = std::move(stderr_text);
    build_artifact_value.compile_failed_run_result_ = std::move(run_result_value);
    return build_artifact_value;
}

std::expected<pl_runner_util::build_artifact, sandbox_error> pl_runner_util::build_source(
    const std::filesystem::path& source_file_path
){
    const std::string extension = source_file_path.extension().string();
    if(extension == ".cpp"){
        if(!cpp_compiler_path_.has_value() || cpp_compiler_path_->empty()){
            return std::unexpected(sandbox_error::invalid_argument);
        }

        return cpp_runner::build(source_file_path, *cpp_compiler_path_);
    }

    if(extension == ".py"){
        return python_runner::build(source_file_path);
    }

    if(extension == ".java"){
        if(
            !java_compiler_path_.has_value() ||
            java_compiler_path_->empty()
        ){
            return std::unexpected(sandbox_error::invalid_argument);
        }

        return java_runner::build(
            source_file_path,
            *java_compiler_path_
        );
    }

    return std::unexpected(sandbox_error::invalid_argument);
}

std::expected<pl_runner_util::execution_plan, sandbox_error> pl_runner_util::make_execution_plan(
    const build_artifact& build_artifact_value
){
    if(!build_artifact_value.is_runnable()){
        return std::unexpected(sandbox_error::invalid_argument);
    }

    execution_plan execution_plan_value;

    switch(build_artifact_value.language_){
    case source_language::cpp: {
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

    case source_language::python: {
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

    case source_language::java: {
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

sandbox_runner::run_options pl_runner_util::make_compile_run_options(
    const std::filesystem::path& workspace_host_path,
    sandbox_runner::mount_profile mount_profile
) const{
    sandbox_runner::run_options run_options_value;
    run_options_value.workspace_host_path = workspace_host_path;
    run_options_value.time_limit = COMPILE_TIME_LIMIT;
    run_options_value.memory_limit_mb = COMPILE_MEMORY_LIMIT_MB;
    run_options_value.policy = sandbox_runner::policy_profile::compile;
    run_options_value.mounts = mount_profile;
    return run_options_value;
}
