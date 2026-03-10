#include "pl_runner/python_runner.hpp"

std::expected<python_runner::compile_result, error_code> python_runner::compile(
    const path& source_file_path,
    const path& python_path
){
    compile_result compile_result_value;
    compile_result_value.source_file_path_ = source_file_path;
    compile_result_value.run_command_args_.push_back(python_path.string());
    compile_result_value.run_command_args_.push_back(source_file_path.string());
    return compile_result_value;
}

std::expected<sandbox_runner::run_result, error_code> python_runner::run(
    const compile_result& compile_result_value,
    const path& input_path,
    std::chrono::milliseconds time_limit,
    std::int64_t memory_limit_mb
){
    return sandbox_runner::run(
        compile_result_value.run_command_args_,
        input_path,
        time_limit,
        memory_limit_mb
    );
}
