#include "judge_core/infrastructure/cpp_runner.hpp"

#include "judge_core/infrastructure/judge_workspace.hpp"
#include "judge_core/infrastructure/sandbox_runner.hpp"

std::expected<program_build::build_artifact, sandbox_error> cpp_runner::build(
    const path& source_file_path,
    const path& compiler_path,
    const sandbox_runner::run_options& compile_run_options_value
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
        compile_run_options_value
    );
    if(!compile_run_exp){
        return std::unexpected(compile_run_exp.error());
    }

    program_build::build_artifact build_artifact_value;
    build_artifact_value.language_ = program_build::source_language::cpp;
    if(compile_run_exp->exit_code_ != 0){
        build_artifact_value.compile_failure_opt_ = program_build::compile_failure{
            compile_run_exp->exit_code_,
            std::move(compile_run_exp->stderr_text_)
        };
        return build_artifact_value;
    }

    build_artifact_value.entry_host_path_ = binary_host_path;
    return build_artifact_value;
}
