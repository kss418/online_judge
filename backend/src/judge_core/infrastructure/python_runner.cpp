#include "judge_core/infrastructure/python_runner.hpp"

std::expected<program_build::build_artifact, sandbox_error> python_runner::build(
    const path& source_file_path
){
    if(source_file_path.empty()){
        return std::unexpected(sandbox_error::invalid_argument);
    }

    program_build::build_artifact build_artifact_value;
    build_artifact_value.language_ = program_build::source_language::python;
    build_artifact_value.entry_host_path_ = source_file_path;
    return build_artifact_value;
}
