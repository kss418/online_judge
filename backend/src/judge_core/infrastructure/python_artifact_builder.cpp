#include "judge_core/infrastructure/python_artifact_builder.hpp"

#include "judge_core/infrastructure/program_build_support.hpp"

std::expected<program_build::build_artifact, sandbox_error> python_artifact_builder::build(
    const path& source_file_path
){
    if(source_file_path.empty()){
        return std::unexpected(sandbox_error::invalid_argument);
    }

    const auto workspace_host_path = source_file_path.parent_path();
    auto build_artifact_value = program_build_support::make_build_artifact_base(
        program_build::source_language::python,
        workspace_host_path
    );
    build_artifact_value.entry_file_host_path_ = source_file_path;
    return build_artifact_value;
}
