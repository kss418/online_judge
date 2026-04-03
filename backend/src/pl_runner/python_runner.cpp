#include "pl_runner/python_runner.hpp"

std::expected<pl_runner_util::build_artifact, sandbox_error> python_runner::build(
    const path& source_file_path
){
    if(source_file_path.empty()){
        return std::unexpected(sandbox_error::invalid_argument);
    }

    pl_runner_util::build_artifact build_artifact_value;
    build_artifact_value.language_ = pl_runner_util::source_language::python;
    build_artifact_value.entry_host_path_ = source_file_path;
    return build_artifact_value;
}
