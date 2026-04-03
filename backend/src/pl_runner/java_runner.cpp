#include "pl_runner/java_runner.hpp"

#include "judge_core/judge_workspace.hpp"
#include "judge_core/sandbox_runner.hpp"

std::expected<pl_runner_util::build_artifact, sandbox_error> java_runner::build(
    const path& source_file_path,
    const path& java_compiler_path
){
    const path workspace_host_path = source_file_path.parent_path();
    const path source_sandbox_path = judge_workspace::make_sandbox_path(
        workspace_host_path,
        source_file_path
    );
    if(source_sandbox_path.empty()){
        return std::unexpected(sandbox_error::invalid_argument);
    }

    const auto run_options_value = pl_runner_util::instance().make_compile_run_options(
        workspace_host_path,
        sandbox_runner::mount_profile::java_profile
    );

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

    if(compile_run_exp->exit_code_ != 0){
        return pl_runner_util::instance().make_compile_failed_artifact(
            pl_runner_util::source_language::java,
            compile_run_exp->exit_code_,
            std::move(compile_run_exp->stderr_text_)
        );
    }

    pl_runner_util::build_artifact build_artifact_value;
    build_artifact_value.language_ = pl_runner_util::source_language::java;
    build_artifact_value.entry_host_path_ = workspace_host_path;
    build_artifact_value.entry_name_ = "Main";
    return build_artifact_value;
}
