#include "pl_runner/pl_runner_utility.hpp"

#include "pl_runner/cpp_runner.hpp"
#include "pl_runner/java_runner.hpp"
#include "pl_runner/python_runner.hpp"

#include <utility>

pl_runner_utility::prepared_source pl_runner_utility::make_compile_failed_prepared_source(
    int exit_code,
    std::string stderr_text
){
    prepared_source prepared_source_value;
    sandbox_runner::run_result run_result_value;
    run_result_value.exit_code_ = exit_code;
    run_result_value.stderr_text_ = std::move(stderr_text);
    prepared_source_value.compile_failed_run_result_ = std::move(run_result_value);
    return prepared_source_value;
}

std::expected<pl_runner_utility::prepared_source, error_code> pl_runner_utility::prepare_source(
    const std::filesystem::path& source_file_path,
    const std::filesystem::path& cpp_compiler_path,
    const std::filesystem::path& python_path,
    const std::filesystem::path& java_runtime_path
){
    const std::string extension = source_file_path.extension().string();
    if(extension == ".cpp"){
        return cpp_runner::prepare(source_file_path, cpp_compiler_path);
    }

    if(extension == ".py"){
        return python_runner::prepare(source_file_path, python_path);
    }

    if(extension == ".java"){
        return java_runner::prepare(source_file_path, java_runtime_path);
    }

    return std::unexpected(error_code::create(errno_error::invalid_argument));
}
