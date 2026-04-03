#include "pl_runner/python_runner.hpp"

#include "judge_core/judge_util.hpp"

std::expected<python_runner::compile_result, sandbox_error> python_runner::compile(
    const path& source_file_path,
    const path& python_path
){
    compile_result compile_result_value;
    compile_result_value.workspace_host_path_ = source_file_path.parent_path();
    const auto source_sandbox_path = judge_util::instance().make_sandbox_path(
        compile_result_value.workspace_host_path_,
        source_file_path
    );
    if(source_sandbox_path.empty()){
        return std::unexpected(sandbox_error::invalid_argument);
    }
    compile_result_value.run_command_args_.push_back(python_path.string());
    compile_result_value.run_command_args_.push_back(source_sandbox_path.string());
    return compile_result_value;
}

std::expected<pl_runner_util::prepared_source, sandbox_error> python_runner::prepare(
    const path& source_file_path,
    const path& python_path
){
    auto compile_python_exp = compile(source_file_path, python_path);
    if(!compile_python_exp){
        return std::unexpected(compile_python_exp.error());
    }

    if(!compile_python_exp->is_success()){
        return pl_runner_util::instance().make_compile_failed_prepared_source(
            compile_python_exp->exit_code_,
            std::move(compile_python_exp->stderr_text_)
        );
    }

    pl_runner_util::prepared_source prepared_source_value;
    prepared_source_value.workspace_host_path_ = compile_python_exp->workspace_host_path_;
    prepared_source_value.run_command_args_ = std::move(compile_python_exp->run_command_args_);
    return prepared_source_value;
}
