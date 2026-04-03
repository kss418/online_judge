#include "pl_runner/cpp_runner.hpp"

#include "judge_core/judge_util.hpp"
#include "judge_core/sandbox_runner.hpp"

namespace{
    constexpr std::chrono::milliseconds COMPILE_TIME_LIMIT{30000};
    constexpr std::int64_t COMPILE_MEMORY_LIMIT_MB = 1024;
}

std::expected<cpp_runner::compile_result, sandbox_error> cpp_runner::compile(
    const path& source_file_path,
    const path& compiler_path
){
    const path workspace_host_path = source_file_path.parent_path();
    const path binary_host_path = workspace_host_path / "program.out";
    const path binary_sandbox_path = judge_util::instance().make_sandbox_path(
        workspace_host_path,
        binary_host_path
    );
    const path source_sandbox_path = judge_util::instance().make_sandbox_path(
        workspace_host_path,
        source_file_path
    );
    if(binary_sandbox_path.empty() || source_sandbox_path.empty()){
        return std::unexpected(sandbox_error::invalid_argument);
    }

    sandbox_runner::run_options run_options_value;
    run_options_value.workspace_host_path = workspace_host_path;
    run_options_value.time_limit = COMPILE_TIME_LIMIT;
    run_options_value.memory_limit_mb = COMPILE_MEMORY_LIMIT_MB;
    run_options_value.policy = sandbox_runner::policy_profile::compile;

    const auto compile_run_exp = sandbox_runner::run(
        {
            compiler_path.string(),
            "-std=c++23",
            "-O2",
            "-pipe",
            source_sandbox_path.string(),
            "-o",
            binary_sandbox_path.string()
        },
        run_options_value
    );
    if(!compile_run_exp){
        return std::unexpected(compile_run_exp.error());
    }

    compile_result compile_result_value;
    compile_result_value.workspace_host_path_ = workspace_host_path;
    compile_result_value.run_command_args_.push_back(binary_sandbox_path.string());
    compile_result_value.exit_code_ = compile_run_exp->exit_code_;
    compile_result_value.stderr_text_ = std::move(compile_run_exp->stderr_text_);
    return compile_result_value;
}

std::expected<pl_runner_util::prepared_source, sandbox_error> cpp_runner::prepare(
    const path& source_file_path,
    const path& compiler_path
){
    auto compile_cpp_exp = compile(source_file_path, compiler_path);
    if(!compile_cpp_exp){
        return std::unexpected(compile_cpp_exp.error());
    }

    if(!compile_cpp_exp->is_success()){
        return pl_runner_util::instance().make_compile_failed_prepared_source(
            compile_cpp_exp->exit_code_,
            std::move(compile_cpp_exp->stderr_text_)
        );
    }

    pl_runner_util::prepared_source prepared_source_value;
    prepared_source_value.workspace_host_path_ = compile_cpp_exp->workspace_host_path_;
    prepared_source_value.run_command_args_ = std::move(compile_cpp_exp->run_command_args_);
    return prepared_source_value;
}
