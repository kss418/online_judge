#include "judge_server/testcase_runner.hpp"

#include "common/file_util.hpp"
#include "dto/problem_dto.hpp"
#include "judge_server/testcase_util.hpp"

#include <chrono>
#include <utility>
#include <vector>

std::expected<problem_dto::limits, error_code> testcase_runner::read_problem_limits(
    std::int64_t problem_id
){
    const auto time_limit_file_path_exp = testcase_util::instance().make_testcase_time_limit_file_path(
        problem_id
    );
    if(!time_limit_file_path_exp){
        return std::unexpected(time_limit_file_path_exp.error());
    }

    const auto memory_limit_file_path_exp = testcase_util::instance().make_testcase_memory_limit_file_path(
        problem_id
    );
    if(!memory_limit_file_path_exp){
        return std::unexpected(memory_limit_file_path_exp.error());
    }

    const auto time_limit_ms_exp = file_util::read_int32_file(*time_limit_file_path_exp);
    if(!time_limit_ms_exp){
        return std::unexpected(time_limit_ms_exp.error());
    }

    const auto memory_limit_mb_exp = file_util::read_int32_file(
        *memory_limit_file_path_exp
    );
    if(!memory_limit_mb_exp){
        return std::unexpected(memory_limit_mb_exp.error());
    }

    problem_dto::limits problem_limits_value;
    problem_limits_value.memory_mb = *memory_limit_mb_exp;
    problem_limits_value.time_ms = *time_limit_ms_exp;
    return problem_limits_value;
}

std::expected<sandbox_runner::run_result, error_code> testcase_runner::run_one_testcase(
    const pl_runner_util::prepared_source& prepared_source_value,
    const std::filesystem::path& input_path,
    const problem_dto::limits& problem_limits_value
){
    if(!prepared_source_value.is_runnable()){
        return *prepared_source_value.compile_failed_run_result_;
    }

    return sandbox_runner::run(
        prepared_source_value.run_command_args_,
        input_path,
        std::chrono::milliseconds{problem_limits_value.time_ms},
        problem_limits_value.memory_mb
    );
}

std::expected<std::vector<sandbox_runner::run_result>, error_code> testcase_runner::run_all_testcases(
    const std::filesystem::path& source_file_path,
    std::int64_t problem_id
){
    const auto testcase_count_exp = testcase_util::instance().count_testcase_output(problem_id);
    if(!testcase_count_exp){
        return std::unexpected(testcase_count_exp.error());
    }

    const auto validated_testcase_count_exp = testcase_util::instance().validate_testcase_output(
        problem_id,
        testcase_count_exp.value()
    );
    if(!validated_testcase_count_exp){
        return std::unexpected(validated_testcase_count_exp.error());
    }

    const auto prepare_source_exp = pl_runner_util::instance().prepare_source(source_file_path);
    if(!prepare_source_exp){
        return std::unexpected(prepare_source_exp.error());
    }

    if(!prepare_source_exp->is_runnable()){
        std::vector<sandbox_runner::run_result> run_results;
        run_results.push_back(*prepare_source_exp->compile_failed_run_result_);
        return run_results;
    }

    const auto problem_limits_exp = read_problem_limits(problem_id);
    if(!problem_limits_exp){
        return std::unexpected(problem_limits_exp.error());
    }

    std::vector<sandbox_runner::run_result> run_results;
    run_results.reserve(static_cast<std::size_t>(*validated_testcase_count_exp));

    for(std::int32_t order = 1; order <= *validated_testcase_count_exp; ++order){
        const auto input_path_exp = testcase_util::instance().make_testcase_input_path(problem_id, order);
        if(!input_path_exp){
            return std::unexpected(input_path_exp.error());
        }

        const auto run_one_testcase_exp = run_one_testcase(
            *prepare_source_exp,
            *input_path_exp,
            *problem_limits_exp
        );
        if(!run_one_testcase_exp){
            return std::unexpected(run_one_testcase_exp.error());
        }

        run_results.push_back(std::move(*run_one_testcase_exp));
    }

    return run_results;
}
