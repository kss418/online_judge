#include "judge_server/testcase_runner.hpp"

#include "common/env_utility.hpp"
#include "common/file_utility.hpp"
#include "pl_runner/cpp_runner.hpp"
#include "pl_runner/java_runner.hpp"
#include "pl_runner/python_runner.hpp"

#include <string>
#include <utility>
#include <vector>

std::expected<testcase_runner, error_code> testcase_runner::create(){
    const auto env_values_exp = env_utility::require_envs(
        {"JUDGE_CPP_COMPILER_PATH", "JUDGE_PYTHON_PATH", "JUDGE_JAVA_RUNTIME_PATH"}
    );
    if(!env_values_exp){
        return std::unexpected(env_values_exp.error());
    }

    return testcase_runner(
        std::move(env_values_exp->at(0)),
        std::move(env_values_exp->at(1)),
        std::move(env_values_exp->at(2))
    );
}

testcase_runner::testcase_runner(
    std::string cpp_compiler_path,
    std::string python_path,
    std::string java_runtime_path
) :
    cpp_compiler_path_(std::move(cpp_compiler_path)),
    python_path_(std::move(python_path)),
    java_runtime_path_(std::move(java_runtime_path)){}

std::expected<std::filesystem::path, error_code> testcase_runner::make_input_path(
    std::int64_t problem_id,
    std::int32_t order
){
    return file_utility::instance().make_testcase_input_path(problem_id, order);
}

std::expected<std::filesystem::path, error_code> testcase_runner::make_output_path(
    std::int64_t problem_id,
    std::int32_t order
){
    return file_utility::instance().make_testcase_output_path(problem_id, order);
}

std::expected<sandbox_runner::run_result, error_code> testcase_runner::run_cpp(
    const std::filesystem::path& source_file_path,
    const std::filesystem::path& input_path
){
    auto compile_cpp_exp = cpp_runner::compile(source_file_path, cpp_compiler_path_);
    if(!compile_cpp_exp){
        return std::unexpected(compile_cpp_exp.error());
    }

    if(!compile_cpp_exp->is_success()){
        sandbox_runner::run_result run_result_value;
        run_result_value.exit_code_ = compile_cpp_exp->exit_code_;
        run_result_value.stderr_text_ = std::move(compile_cpp_exp->stderr_text_);
        return run_result_value;
    }

    return cpp_runner::run(
        *compile_cpp_exp,
        input_path,
        source_run_time_limit_,
        source_run_memory_limit_mb_
    );
}

std::expected<sandbox_runner::run_result, error_code> testcase_runner::run_python(
    const std::filesystem::path& source_file_path,
    const std::filesystem::path& input_path
){
    auto compile_python_exp = python_runner::compile(source_file_path, python_path_);
    if(!compile_python_exp){
        return std::unexpected(compile_python_exp.error());
    }

    if(!compile_python_exp->is_success()){
        sandbox_runner::run_result run_result_value;
        run_result_value.exit_code_ = compile_python_exp->exit_code_;
        run_result_value.stderr_text_ = std::move(compile_python_exp->stderr_text_);
        return run_result_value;
    }

    return python_runner::run(
        *compile_python_exp,
        input_path,
        source_run_time_limit_,
        source_run_memory_limit_mb_
    );
}

std::expected<sandbox_runner::run_result, error_code> testcase_runner::run_java(
    const std::filesystem::path& source_file_path,
    const std::filesystem::path& input_path
){
    auto compile_java_exp = java_runner::compile(source_file_path, java_runtime_path_);
    if(!compile_java_exp){
        return std::unexpected(compile_java_exp.error());
    }

    if(!compile_java_exp->is_success()){
        sandbox_runner::run_result run_result_value;
        run_result_value.exit_code_ = compile_java_exp->exit_code_;
        run_result_value.stderr_text_ = std::move(compile_java_exp->stderr_text_);
        return run_result_value;
    }

    return java_runner::run(
        *compile_java_exp,
        input_path,
        source_run_time_limit_,
        source_run_memory_limit_mb_
    );
}

std::expected<std::vector<sandbox_runner::run_result>, error_code> testcase_runner::run_cpp(
    const std::filesystem::path& source_file_path,
    std::int64_t problem_id,
    std::int32_t testcase_count
){
    std::vector<sandbox_runner::run_result> run_results;
    run_results.reserve(static_cast<std::size_t>(testcase_count));

    auto compile_cpp_exp = cpp_runner::compile(source_file_path, cpp_compiler_path_);
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

    for(std::int32_t order = 1; order <= testcase_count; ++order){
        const auto input_path_exp = make_input_path(problem_id, order);
        if(!input_path_exp){
            return std::unexpected(input_path_exp.error());
        }

        const auto output_path_exp = make_output_path(problem_id, order);
        if(!output_path_exp){
            return std::unexpected(output_path_exp.error());
        }
        (void)output_path_exp;

        const auto run_cpp_exp = cpp_runner::run(
            *compile_cpp_exp,
            *input_path_exp,
            source_run_time_limit_,
            source_run_memory_limit_mb_
        );
        if(!run_cpp_exp){
            return std::unexpected(run_cpp_exp.error());
        }

        run_results.push_back(std::move(*run_cpp_exp));
    }

    return run_results;
}

std::expected<std::vector<sandbox_runner::run_result>, error_code> testcase_runner::run_python(
    const std::filesystem::path& source_file_path,
    std::int64_t problem_id,
    std::int32_t testcase_count
){
    std::vector<sandbox_runner::run_result> run_results;
    run_results.reserve(static_cast<std::size_t>(testcase_count));

    auto compile_python_exp = python_runner::compile(source_file_path, python_path_);
    if(!compile_python_exp){
        return std::unexpected(compile_python_exp.error());
    }

    if(!compile_python_exp->is_success()){
        sandbox_runner::run_result run_result_value;
        run_result_value.exit_code_ = compile_python_exp->exit_code_;
        run_result_value.stderr_text_ = std::move(compile_python_exp->stderr_text_);
        run_results.push_back(std::move(run_result_value));
        return run_results;
    }

    for(std::int32_t order = 1; order <= testcase_count; ++order){
        const auto input_path_exp = make_input_path(problem_id, order);
        if(!input_path_exp){
            return std::unexpected(input_path_exp.error());
        }

        const auto output_path_exp = make_output_path(problem_id, order);
        if(!output_path_exp){
            return std::unexpected(output_path_exp.error());
        }
        (void)output_path_exp;

        const auto run_python_exp = python_runner::run(
            *compile_python_exp,
            *input_path_exp,
            source_run_time_limit_,
            source_run_memory_limit_mb_
        );
        if(!run_python_exp){
            return std::unexpected(run_python_exp.error());
        }

        run_results.push_back(std::move(*run_python_exp));
    }

    return run_results;
}

std::expected<std::vector<sandbox_runner::run_result>, error_code> testcase_runner::run_java(
    const std::filesystem::path& source_file_path,
    std::int64_t problem_id,
    std::int32_t testcase_count
){
    std::vector<sandbox_runner::run_result> run_results;
    run_results.reserve(static_cast<std::size_t>(testcase_count));

    auto compile_java_exp = java_runner::compile(source_file_path, java_runtime_path_);
    if(!compile_java_exp){
        return std::unexpected(compile_java_exp.error());
    }

    if(!compile_java_exp->is_success()){
        sandbox_runner::run_result run_result_value;
        run_result_value.exit_code_ = compile_java_exp->exit_code_;
        run_result_value.stderr_text_ = std::move(compile_java_exp->stderr_text_);
        run_results.push_back(std::move(run_result_value));
        return run_results;
    }

    for(std::int32_t order = 1; order <= testcase_count; ++order){
        const auto input_path_exp = make_input_path(problem_id, order);
        if(!input_path_exp){
            return std::unexpected(input_path_exp.error());
        }

        const auto output_path_exp = make_output_path(problem_id, order);
        if(!output_path_exp){
            return std::unexpected(output_path_exp.error());
        }
        (void)output_path_exp;

        const auto run_java_exp = java_runner::run(
            *compile_java_exp,
            *input_path_exp,
            source_run_time_limit_,
            source_run_memory_limit_mb_
        );
        if(!run_java_exp){
            return std::unexpected(run_java_exp.error());
        }

        run_results.push_back(std::move(*run_java_exp));
    }

    return run_results;
}

std::expected<sandbox_runner::run_result, error_code> testcase_runner::run_one_testcase(
    const std::filesystem::path& source_file_path,
    const std::filesystem::path& input_path
){
    const std::string extension = source_file_path.extension().string();
    if(extension == ".cpp"){
        return run_cpp(source_file_path, input_path);
    }

    if(extension == ".py"){
        return run_python(source_file_path, input_path);
    }

    if(extension == ".java"){
        return run_java(source_file_path, input_path);
    }

    return std::unexpected(error_code::create(errno_error::invalid_argument));
}

std::expected<std::vector<sandbox_runner::run_result>, error_code> testcase_runner::run_all_testcases(
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

    if(extension == ".cpp"){
        return run_cpp(source_file_path, problem_id, *validated_testcase_count_exp);
    }

    if(extension == ".py"){
        return run_python(source_file_path, problem_id, *validated_testcase_count_exp);
    }

    if(extension == ".java"){
        return run_java(source_file_path, problem_id, *validated_testcase_count_exp);
    }

    return std::unexpected(error_code::create(errno_error::invalid_argument));
}
