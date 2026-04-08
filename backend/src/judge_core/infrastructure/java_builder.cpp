#include "judge_core/infrastructure/java_builder.hpp"

#include "judge_core/infrastructure/program_build_support.hpp"
#include "judge_core/infrastructure/sandbox_runner.hpp"
#include "judge_core/types/workspace_layout.hpp"

std::expected<program_build::build_artifact, sandbox_error> java_builder::build(
    const path& source_file_path,
    const path& java_compiler_path
){
    const auto workspace_host_path = source_file_path.parent_path();
    const auto workspace_layout_exp = workspace_layout::create(
        workspace_host_path
    );
    if(!workspace_layout_exp){
        return std::unexpected(sandbox_error::invalid_argument);
    }

    const auto source_sandbox_path_exp = workspace_layout_exp->sandbox_path_for(
        source_file_path
    );
    if(java_compiler_path.empty() || !source_sandbox_path_exp){
        return std::unexpected(sandbox_error::invalid_argument);
    }

    const auto compile_run_exp = sandbox_runner::run(
        {
            java_compiler_path.string(),
            "-d",
            workspace_layout_exp->sandbox_root().string(),
            source_sandbox_path_exp->string()
        },
        program_build_support::make_compile_run_options(
            workspace_host_path,
            sandbox_runner::mount_profile::java_profile
        )
    );
    if(!compile_run_exp){
        return std::unexpected(compile_run_exp.error());
    }

    if(compile_run_exp->exit_code_ != 0){
        return program_build_support::make_compile_failure_artifact(
            program_build::source_language::java,
            workspace_host_path,
            std::move(*compile_run_exp)
        );
    }

    auto build_artifact_value = program_build_support::make_build_artifact_base(
        program_build::source_language::java,
        workspace_host_path
    );
    build_artifact_value.main_class_name_ = "Main";
    return build_artifact_value;
}
