#include "pl_runner/python_runner.hpp"

std::expected<python_runner::compile_result, error_code> python_runner::compile(
    const path& source_file_path,
    const path& python_path
){
    compile_result compile_result_value;
    compile_result_value.run_command_args_.push_back(python_path.string());
    compile_result_value.run_command_args_.push_back(source_file_path.string());
    return compile_result_value;
}

std::expected<pl_runner_utility::prepared_source, error_code> python_runner::prepare(
    const path& source_file_path,
    const path& python_path
){
    auto compile_python_exp = compile(source_file_path, python_path);
    if(!compile_python_exp){
        return std::unexpected(compile_python_exp.error());
    }

    if(!compile_python_exp->is_success()){
        return pl_runner_utility::instance().make_compile_failed_prepared_source(
            compile_python_exp->exit_code_,
            std::move(compile_python_exp->stderr_text_)
        );
    }

    pl_runner_utility::prepared_source prepared_source_value;
    prepared_source_value.run_command_args_ = std::move(compile_python_exp->run_command_args_);
    return prepared_source_value;
}
