#include "pl_runner/python_runner.hpp"

std::expected<program_build::runner_build_result, sandbox_error> python_runner::build(
    const path& source_file_path
){
    if(source_file_path.empty()){
        return std::unexpected(sandbox_error::invalid_argument);
    }

    program_build::runner_build_result runner_build_result_value;
    runner_build_result_value.entry_host_path_ = source_file_path;
    return runner_build_result_value;
}
