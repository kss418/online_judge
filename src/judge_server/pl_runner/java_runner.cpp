#include "judge_server/pl_runner/java_runner.hpp"

std::expected<java_runner::compile_result, error_code> java_runner::compile(
    const path& source_file_path,
    const path& java_runtime_path
){
    compile_result compile_result_value;
    compile_result_value.source_file_path_ = source_file_path;
    compile_result_value.run_command_args_.push_back(java_runtime_path.string());
    compile_result_value.run_command_args_.push_back(source_file_path.string());
    return compile_result_value;
}

std::expected<sandbox_runner::run_result, error_code> java_runner::run(
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
