#include "judge_server/code_runner.hpp"

#include "common/env_utility.hpp"
#include "common/file_utility.hpp"
#include "common/temp_file.hpp"
#include "judge_server/compile_runner.hpp"

#include <optional>
#include <string>
#include <utility>
#include <vector>

std::expected<code_runner, error_code> code_runner::create(){
    const auto env_values_exp = env_utility::require_envs(
        {"JUDGE_CPP_COMPILER_PATH", "JUDGE_PYTHON_PATH", "JUDGE_JAVA_RUNTIME_PATH"}
    );
    if(!env_values_exp){
        return std::unexpected(env_values_exp.error());
    }

    return code_runner(
        std::move(env_values_exp->at(0)),
        std::move(env_values_exp->at(1)),
        std::move(env_values_exp->at(2))
    );
}

code_runner::code_runner(
    std::string cpp_compiler_path,
    std::string python_path,
    std::string java_runtime_path
) :
    cpp_compiler_path_(std::move(cpp_compiler_path)),
    python_path_(std::move(python_path)),
    java_runtime_path_(std::move(java_runtime_path)){}

std::expected<std::filesystem::path, error_code> code_runner::make_input_path(
    std::int64_t problem_id,
    std::int32_t order
){
    return file_utility::instance().make_testcase_input_path(problem_id, order);
}

std::expected<std::filesystem::path, error_code> code_runner::make_output_path(
    std::int64_t problem_id,
    std::int32_t order
){
    return file_utility::instance().make_testcase_output_path(problem_id, order);
}

std::expected<sandbox_runner::run_result, error_code> code_runner::run_one_testcase(
    const std::filesystem::path& source_file_path,
    const std::filesystem::path& input_path
){
    const std::string extension = source_file_path.extension().string();
    if(extension == ".cpp"){
        auto compile_cpp_exp = compile_cpp(source_file_path);
        if(!compile_cpp_exp){
            return std::unexpected(compile_cpp_exp.error());
        }

        if(!compile_cpp_exp->is_success()){
            sandbox_runner::run_result run_result_value;
            run_result_value.exit_code_ = compile_cpp_exp->exit_code_;
            run_result_value.stderr_text_ = std::move(compile_cpp_exp->stderr_text_);
            return run_result_value;
        }

        if(!compile_cpp_exp->artifact_file_.has_value()){
            return std::unexpected(error_code::create(errno_error::invalid_argument));
        }

        return run_cpp(compile_cpp_exp->artifact_file_->get_path(), input_path);
    }
    if(extension == ".py"){
        return run_python(source_file_path, input_path);
    }
    if(extension == ".java"){
        return run_java(source_file_path, input_path);
    }

    return std::unexpected(error_code::create(errno_error::invalid_argument));
}

std::expected<std::vector<sandbox_runner::run_result>, error_code> code_runner::run_all_testcases(
    const std::filesystem::path& source_file_path,
    std::int64_t problem_id
){
    const std::string extension = source_file_path.extension().string();
    const auto testcase_count_exp = file_utility::instance().count_testcase_output(problem_id);
    if(!testcase_count_exp){
        return std::unexpected(testcase_count_exp.error());
    }

    const auto validated_testcase_count_exp = file_utility::instance().validate_testcase_output(
        problem_id,
        testcase_count_exp.value()
    );
    if(!validated_testcase_count_exp){
        return std::unexpected(validated_testcase_count_exp.error());
    }

    std::vector<sandbox_runner::run_result> run_results;
    run_results.reserve(static_cast<std::size_t>(validated_testcase_count_exp.value()));

    std::optional<compile_result> compile_result_value;
    if(extension == ".cpp"){
        auto compile_cpp_exp = compile_cpp(source_file_path);
        if(!compile_cpp_exp){
            return std::unexpected(compile_cpp_exp.error());
        }

        if(!compile_cpp_exp->is_success()){
            sandbox_runner::run_result run_result_value;
            run_result_value.exit_code_ = compile_cpp_exp->exit_code_;
            run_result_value.stderr_text_ = std::move(compile_cpp_exp->stderr_text_);
            run_results.push_back(std::move(run_result_value));
            return run_results;
        }

        compile_result_value = std::move(*compile_cpp_exp);
    }

    for(std::int32_t order = 1; order <= validated_testcase_count_exp.value(); ++order){
        const auto input_path_exp = make_input_path(problem_id, order);
        if(!input_path_exp){
            return std::unexpected(input_path_exp.error());
        }

        const auto output_path_exp = make_output_path(problem_id, order);
        if(!output_path_exp){
            return std::unexpected(output_path_exp.error());
        }
        (void)output_path_exp;

        if(extension == ".cpp"){
            if(!compile_result_value->artifact_file_.has_value()){
                return std::unexpected(error_code::create(errno_error::invalid_argument));
            }

            const auto run_cpp_exp = run_cpp(
                compile_result_value->artifact_file_->get_path(),
                *input_path_exp
            );
            if(!run_cpp_exp){
                return std::unexpected(run_cpp_exp.error());
            }

            run_results.push_back(std::move(*run_cpp_exp));
            continue;
        }

        const auto run_one_testcase_exp = run_one_testcase(source_file_path, *input_path_exp);
        if(!run_one_testcase_exp){
            return std::unexpected(run_one_testcase_exp.error());
        }

        run_results.push_back(std::move(*run_one_testcase_exp));
    }

    return run_results;
}

std::expected<code_runner::compile_result, error_code> code_runner::compile_cpp(
    const std::filesystem::path& source_file_path
){
    auto binary_file_exp = temp_file::create("/tmp/oj_binary_XXXXXX");
    if(!binary_file_exp){
        return std::unexpected(binary_file_exp.error());
    }

    auto compile_exp = compile_runner::compile_cpp(
        source_file_path,
        binary_file_exp->get_path(),
        cpp_compiler_path_
    );
    if(!compile_exp){
        return std::unexpected(compile_exp.error());
    }

    binary_file_exp->close_fd();

    compile_result compile_result_value;
    compile_result_value.artifact_file_ = std::move(*binary_file_exp);
    compile_result_value.run_command_args_.push_back(
        compile_result_value.artifact_file_->get_path().string()
    );
    compile_result_value.exit_code_ = compile_exp->exit_code_;
    compile_result_value.stderr_text_ = std::move(compile_exp->stderr_text_);
    return compile_result_value;
}

std::expected<sandbox_runner::run_result, error_code> code_runner::run_cpp(
    const std::filesystem::path& binary_file_path,
    const std::filesystem::path& input_path
){
    std::vector<std::string> command_args = {binary_file_path.string()};
    return sandbox_runner::run(
        command_args,
        input_path,
        source_run_time_limit_,
        source_run_memory_limit_mb_
    );
}

std::expected<sandbox_runner::run_result, error_code> code_runner::run_python(
    const std::filesystem::path& source_file_path,
    const std::filesystem::path& input_path
){
    std::vector<std::string> command_args = {python_path_, source_file_path.string()};
    return sandbox_runner::run(
        command_args,
        input_path,
        source_run_time_limit_,
        source_run_memory_limit_mb_
    );
}

std::expected<sandbox_runner::run_result, error_code> code_runner::run_java(
    const std::filesystem::path& source_file_path,
    const std::filesystem::path& input_path
){
    std::vector<std::string> command_args = {java_runtime_path_, source_file_path.string()};
    return sandbox_runner::run(
        command_args,
        input_path,
        source_run_time_limit_,
        source_run_memory_limit_mb_
    );
}
