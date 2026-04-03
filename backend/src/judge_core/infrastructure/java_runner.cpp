#include "judge_core/infrastructure/java_runner.hpp"

#include "judge_core/infrastructure/judge_workspace.hpp"
#include "judge_core/infrastructure/sandbox_runner.hpp"

std::expected<program_build::runner_build_result, sandbox_error> java_runner::build(
    const path& source_file_path,
    const path& java_compiler_path,
    const sandbox_runner::run_options& compile_run_options_value
){
    const path workspace_host_path = source_file_path.parent_path();
    const path source_sandbox_path = judge_workspace::make_sandbox_path(
        workspace_host_path,
        source_file_path
    );
    if(source_sandbox_path.empty()){
        return std::unexpected(sandbox_error::invalid_argument);
    }

    const auto compile_run_exp = sandbox_runner::run(
        {
            java_compiler_path.string(),
            "-d",
            judge_workspace::sandbox_workspace_path().string(),
            source_sandbox_path.string()
        },
        compile_run_options_value
    );
    if(!compile_run_exp){
        return std::unexpected(compile_run_exp.error());
    }

    program_build::runner_build_result runner_build_result_value;
    if(compile_run_exp->exit_code_ != 0){
        runner_build_result_value.compile_failure_opt_ = program_build::compile_failure{
            compile_run_exp->exit_code_,
            std::move(compile_run_exp->stderr_text_)
        };
        return runner_build_result_value;
    }

    runner_build_result_value.entry_host_path_ = workspace_host_path;
    runner_build_result_value.entry_name_ = "Main";
    return runner_build_result_value;
}
