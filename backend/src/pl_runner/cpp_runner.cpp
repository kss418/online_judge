#include "pl_runner/cpp_runner.hpp"

#include "judge_core/judge_workspace.hpp"
#include "judge_core/sandbox_runner.hpp"

std::expected<pl_runner_util::build_artifact, sandbox_error> cpp_runner::build(
    const path& source_file_path,
    const path& compiler_path
){
    const path workspace_host_path = source_file_path.parent_path();
    const path binary_host_path = workspace_host_path / "program.out";
    const path binary_sandbox_path = judge_workspace::make_sandbox_path(
        workspace_host_path,
        binary_host_path
    );
    const path source_sandbox_path = judge_workspace::make_sandbox_path(
        workspace_host_path,
        source_file_path
    );
    if(binary_sandbox_path.empty() || source_sandbox_path.empty()){
        return std::unexpected(sandbox_error::invalid_argument);
    }

    const auto run_options_value =
        pl_runner_util::instance().make_compile_run_options(workspace_host_path);

    const auto compile_run_exp = sandbox_runner::run(
        {
            compiler_path.string(),
            "-std=c++23",
            "-O2",
            "-pipe",
            source_sandbox_path.string(),
            "-o",
            binary_sandbox_path.string()
        },
        run_options_value
    );
    if(!compile_run_exp){
        return std::unexpected(compile_run_exp.error());
    }

    if(compile_run_exp->exit_code_ != 0){
        return pl_runner_util::instance().make_compile_failed_artifact(
            pl_runner_util::source_language::cpp,
            compile_run_exp->exit_code_,
            std::move(compile_run_exp->stderr_text_)
        );
    }

    pl_runner_util::build_artifact build_artifact_value;
    build_artifact_value.language_ = pl_runner_util::source_language::cpp;
    build_artifact_value.entry_host_path_ = binary_host_path;
    return build_artifact_value;
}
