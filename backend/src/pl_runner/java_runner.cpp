#include "pl_runner/java_runner.hpp"

#include "judge_core/judge_workspace.hpp"
#include "judge_core/sandbox_runner.hpp"

namespace{
    constexpr std::chrono::milliseconds COMPILE_TIME_LIMIT{30000};
    constexpr std::int64_t COMPILE_MEMORY_LIMIT_MB = 1024;
}

std::expected<java_runner::compile_result, sandbox_error> java_runner::compile(
    const path& source_file_path,
    const path& java_compiler_path,
    const path& java_runtime_path
){
    const path workspace_host_path = source_file_path.parent_path();
    const path source_sandbox_path = judge_workspace::make_sandbox_path(
        workspace_host_path,
        source_file_path
    );
    if(source_sandbox_path.empty()){
        return std::unexpected(sandbox_error::invalid_argument);
    }

    sandbox_runner::run_options run_options_value;
    run_options_value.workspace_host_path = workspace_host_path;
    run_options_value.time_limit = COMPILE_TIME_LIMIT;
    run_options_value.memory_limit_mb = COMPILE_MEMORY_LIMIT_MB;
    run_options_value.policy = sandbox_runner::policy_profile::compile;
    run_options_value.mounts = sandbox_runner::mount_profile::java_profile;

    const auto compile_run_exp = sandbox_runner::run(
        {
            java_compiler_path.string(),
            "-d",
            judge_workspace::sandbox_workspace_path().string(),
            source_sandbox_path.string()
        },
        run_options_value
    );
    if(!compile_run_exp){
        return std::unexpected(compile_run_exp.error());
    }

    compile_result compile_result_value;
    compile_result_value.workspace_host_path_ = workspace_host_path;
    compile_result_value.exit_code_ = compile_run_exp->exit_code_;
    compile_result_value.stderr_text_ = std::move(compile_run_exp->stderr_text_);
    compile_result_value.run_command_args_.push_back(java_runtime_path.string());
    compile_result_value.run_command_args_.push_back("-XX:-UsePerfData");
    compile_result_value.run_command_args_.push_back("-cp");
    compile_result_value.run_command_args_.push_back(
        judge_workspace::sandbox_workspace_path().string()
    );
    compile_result_value.run_command_args_.push_back("Main");
    return compile_result_value;
}

std::expected<pl_runner_util::prepared_source, sandbox_error> java_runner::prepare(
    const path& source_file_path,
    const path& java_compiler_path,
    const path& java_runtime_path
){
    auto compile_java_exp = compile(
        source_file_path,
        java_compiler_path,
        java_runtime_path
    );
    if(!compile_java_exp){
        return std::unexpected(compile_java_exp.error());
    }

    if(!compile_java_exp->is_success()){
        return pl_runner_util::instance().make_compile_failed_prepared_source(
            compile_java_exp->exit_code_,
            std::move(compile_java_exp->stderr_text_)
        );
    }

    pl_runner_util::prepared_source prepared_source_value;
    prepared_source_value.workspace_host_path_ = compile_java_exp->workspace_host_path_;
    prepared_source_value.run_command_args_ = std::move(compile_java_exp->run_command_args_);
    prepared_source_value.mount_profile_ = sandbox_runner::mount_profile::java_profile;
    return prepared_source_value;
}
