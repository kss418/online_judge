#include "judge_server/testcase_runner.hpp"

#include "common/env_utility.hpp"
#include "common/file_utility.hpp"

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

std::expected<sandbox_runner::run_result, error_code> testcase_runner::run_one_testcase(
    const pl_runner_utility::prepared_source& prepared_source_value,
    const std::filesystem::path& input_path
){
    if(!prepared_source_value.is_runnable()){
        return *prepared_source_value.compile_failed_run_result_;
    }

    return sandbox_runner::run(
        prepared_source_value.run_command_args_,
        input_path,
        source_run_time_limit_,
        source_run_memory_limit_mb_
    );
}

std::expected<std::vector<sandbox_runner::run_result>, error_code> testcase_runner::run_all_testcases(
    const std::filesystem::path& source_file_path,
    std::int64_t problem_id
){
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

    const auto prepare_source_exp = pl_runner_utility::prepare_source(
        source_file_path,
        cpp_compiler_path_,
        python_path_,
        java_runtime_path_
    );
    
    if(!prepare_source_exp){
        return std::unexpected(prepare_source_exp.error());
    }

    if(!prepare_source_exp->is_runnable()){
        std::vector<sandbox_runner::run_result> run_results;
        run_results.push_back(*prepare_source_exp->compile_failed_run_result_);
        return run_results;
    }

    std::vector<sandbox_runner::run_result> run_results;
    run_results.reserve(static_cast<std::size_t>(*validated_testcase_count_exp));

    for(std::int32_t order = 1; order <= *validated_testcase_count_exp; ++order){
        const auto input_path_exp = make_input_path(problem_id, order);
        if(!input_path_exp){
            return std::unexpected(input_path_exp.error());
        }

        const auto run_one_testcase_exp = run_one_testcase(*prepare_source_exp, *input_path_exp);
        if(!run_one_testcase_exp){
            return std::unexpected(run_one_testcase_exp.error());
        }

        run_results.push_back(std::move(*run_one_testcase_exp));
    }

    return run_results;
}
