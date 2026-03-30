#include "judge_core/testcase_runner.hpp"

#include "common/file_util.hpp"
#include "judge_core/testcase_util.hpp"

#include <chrono>
#include <utility>
#include <vector>

std::expected<problem_content_dto::limits, error_code> testcase_runner::read_problem_limits(
    const std::filesystem::path& testcase_directory_path
){
    const auto time_limit_file_path_exp = testcase_util::instance().make_testcase_time_limit_file_path(
        testcase_directory_path
    );
    if(!time_limit_file_path_exp){
        return std::unexpected(time_limit_file_path_exp.error());
    }

    const auto memory_limit_file_path_exp = testcase_util::instance().make_testcase_memory_limit_file_path(
        testcase_directory_path
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

    problem_content_dto::limits problem_limits_value;
    problem_limits_value.memory_mb = *memory_limit_mb_exp;
    problem_limits_value.time_ms = *time_limit_ms_exp;
    return problem_limits_value;
}

std::expected<sandbox_runner::run_result, error_code> testcase_runner::run_one_testcase(
    const pl_runner_util::prepared_source& prepared_source_value,
    const std::filesystem::path& input_path,
    const problem_content_dto::limits& problem_limits_value
){
    if(!prepared_source_value.is_runnable()){
        return *prepared_source_value.compile_failed_run_result_;
    }

    sandbox_runner::run_options run_options_value;
    run_options_value.workspace_host_path = prepared_source_value.workspace_host_path_;
    run_options_value.input_path_opt = input_path;
    run_options_value.time_limit = std::chrono::milliseconds{problem_limits_value.time_ms};
    run_options_value.memory_limit_mb = problem_limits_value.memory_mb;
    run_options_value.policy = sandbox_runner::policy_profile::run;
    run_options_value.mounts = prepared_source_value.mount_profile_;

    return sandbox_runner::run(prepared_source_value.run_command_args_, run_options_value);
}

std::expected<testcase_runner::run_batch, error_code> testcase_runner::run_all_testcases(
    const std::filesystem::path& source_file_path,
    const std::filesystem::path& testcase_directory_path
){
    const auto testcase_count_exp = testcase_util::instance().count_testcase_output(
        testcase_directory_path
    );
    if(!testcase_count_exp){
        return std::unexpected(testcase_count_exp.error());
    }

    const auto validated_testcase_count_exp = testcase_util::instance().validate_testcase_output(
        testcase_directory_path,
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
        run_batch run_batch_value;
        run_batch_value.compile_failed = true;
        run_batch_value.run_results.push_back(*prepare_source_exp->compile_failed_run_result_);
        return run_batch_value;
    }

    const auto problem_limits_exp = read_problem_limits(testcase_directory_path);
    if(!problem_limits_exp){
        return std::unexpected(problem_limits_exp.error());
    }

    run_batch run_batch_value;
    run_batch_value.run_results.reserve(static_cast<std::size_t>(*validated_testcase_count_exp));

    for(std::int32_t order = 1; order <= *validated_testcase_count_exp; ++order){
        const auto input_path_exp = testcase_util::instance().make_testcase_input_path(
            testcase_directory_path,
            order
        );
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

        run_batch_value.run_results.push_back(std::move(*run_one_testcase_exp));
    }

    return run_batch_value;
}
