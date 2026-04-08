#include "judge_core/infrastructure/cpp_builder.hpp"

#include "judge_core/infrastructure/program_build_support.hpp"
#include "judge_core/infrastructure/sandbox_runner.hpp"
#include "judge_core/types/workspace_layout.hpp"

std::expected<program_build::build_artifact, sandbox_error> cpp_builder::build(
    const path& source_file_path,
    const path& compiler_path
){
    const auto workspace_host_path = source_file_path.parent_path();
    const auto binary_host_path = workspace_host_path / "program.out";
    const auto workspace_layout_exp = workspace_layout::create(
        workspace_host_path
    );
    if(!workspace_layout_exp){
        return std::unexpected(sandbox_error::invalid_argument);
    }

    const auto binary_sandbox_path_exp = workspace_layout_exp->sandbox_path_for(
        binary_host_path
    );
    const auto source_sandbox_path_exp = workspace_layout_exp->sandbox_path_for(
        source_file_path
    );
    if(
        compiler_path.empty() ||
        !binary_sandbox_path_exp ||
        !source_sandbox_path_exp
    ){
        return std::unexpected(sandbox_error::invalid_argument);
    }

    const auto compile_run_exp = sandbox_runner::run(
        {
            compiler_path.string(),
            "-std=c++23",
            "-O2",
            "-pipe",
            source_sandbox_path_exp->string(),
            "-o",
            binary_sandbox_path_exp->string()
        },
        program_build_support::make_compile_run_options(workspace_host_path)
    );
    if(!compile_run_exp){
        return std::unexpected(compile_run_exp.error());
    }

    if(!compile_run_exp->is_success()){
        return program_build_support::make_compile_failure_artifact(
            program_build::source_language::cpp,
            workspace_host_path,
            std::move(*compile_run_exp)
        );
    }

    auto build_artifact_value = program_build_support::make_build_artifact_base(
        program_build::source_language::cpp,
        workspace_host_path
    );
    build_artifact_value.entry_file_host_path_ = binary_host_path;
    return build_artifact_value;
}
