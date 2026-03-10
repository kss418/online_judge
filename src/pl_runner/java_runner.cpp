#include "pl_runner/java_runner.hpp"

std::expected<java_runner::compile_result, error_code> java_runner::compile(
    const path& source_file_path,
    const path& java_runtime_path
){
    compile_result compile_result_value;
    compile_result_value.run_command_args_.push_back(java_runtime_path.string());
    compile_result_value.run_command_args_.push_back(source_file_path.string());
    return compile_result_value;
}

std::expected<pl_runner_utility::prepared_source, error_code> java_runner::prepare(
    const path& source_file_path,
    const path& java_runtime_path
){
    auto compile_java_exp = compile(source_file_path, java_runtime_path);
    if(!compile_java_exp){
        return std::unexpected(compile_java_exp.error());
    }

    if(!compile_java_exp->is_success()){
        return pl_runner_utility::instance().make_compile_failed_prepared_source(
            compile_java_exp->exit_code_,
            std::move(compile_java_exp->stderr_text_)
        );
    }

    pl_runner_utility::prepared_source prepared_source_value;
    prepared_source_value.run_command_args_ = std::move(compile_java_exp->run_command_args_);
    return prepared_source_value;
}
