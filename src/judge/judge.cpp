#include "judge/judge.hpp"
#include "judge/checker.hpp"
#include "judge/code_runner.hpp"
#include "judge/compile_runner.hpp"
#include "core/temp_file.hpp"

#include <string>

std::expected <std::pair<judge_result, std::string>, error_code> judge::judge_cpp(
    path source_path, path input_path, path answer_path, path compiler_path
){
    auto binary_temp = temp_file::create("/tmp/oj_binary_XXXXXX");
    if(!binary_temp){
        return std::unexpected(binary_temp.error());
    }

    auto compile_exp = compile_runner::compile_cpp(source_path, binary_temp->get_path(), compiler_path);
    if(!compile_exp){
        return std::unexpected(compile_exp.error());
    }

    if(compile_exp->exit_code_ != 0){
        return std::pair{judge_result::compile_error, compile_exp->stderr_text_};
    }

    binary_temp->close_fd();
    auto run_exp = code_runner::run_cpp(binary_temp->get_path(), input_path);
    if(!run_exp){
        return std::unexpected(run_exp.error());
    }

    auto run_result_value = std::move(*run_exp);
    if(!run_result_value.is_success()){
        std::string runtime_message = run_result_value.stderr_text_;
        if(runtime_message.empty()){
            runtime_message = "process exited with code " + std::to_string(run_result_value.exit_code_);
        }
        return std::pair{judge_result::runtime_error, std::move(runtime_message)};
    }

    auto check_exp = checker::check(std::move(run_result_value.output_lines_), answer_path);
    if(!check_exp){
        return std::unexpected(check_exp.error());
    }

    return std::pair{*check_exp, ""};
}
