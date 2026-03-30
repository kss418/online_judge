#include "judge_core/testcase_runner.hpp"

#include "common/timer.hpp"

#include <utility>
#include <vector>

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
    const testcase_snapshot& testcase_snapshot_value
){
    const auto validated_testcase_count_exp = testcase_snapshot_value.validate_testcase_layout();
    if(!validated_testcase_count_exp){
        return std::unexpected(validated_testcase_count_exp.error());
    }

    run_batch run_batch_value;
    run_batch_value.testcase_count = *validated_testcase_count_exp;

    const auto prepare_source_exp = timer::measure_elapsed_ms(
        run_batch_value.prepare_elapsed_ms,
        [&source_file_path]{
            return pl_runner_util::instance().prepare_source(source_file_path);
        }
    );
    if(!prepare_source_exp){
        return std::unexpected(prepare_source_exp.error());
    }

    if(!prepare_source_exp->is_runnable()){
        run_batch_value.compile_failed = true;
        run_batch_value.run_results.push_back(*prepare_source_exp->compile_failed_run_result_);
        return run_batch_value;
    }

    run_batch_value.run_results.reserve(static_cast<std::size_t>(*validated_testcase_count_exp));

    const auto run_testcases_exp = timer::measure_elapsed_ms(
        run_batch_value.testcase_execution_elapsed_ms,
        [&]() -> std::expected<void, error_code> {
            for(std::int32_t order = 1; order <= *validated_testcase_count_exp; ++order){
                const auto input_path_exp = testcase_snapshot_value.make_input_path(order);
                if(!input_path_exp){
                    return std::unexpected(input_path_exp.error());
                }

                const auto run_one_testcase_exp = run_one_testcase(
                    *prepare_source_exp,
                    *input_path_exp,
                    testcase_snapshot_value.limits_value
                );
                if(!run_one_testcase_exp){
                    return std::unexpected(run_one_testcase_exp.error());
                }

                run_batch_value.run_results.push_back(std::move(*run_one_testcase_exp));
            }

            return {};
        }
    );
    if(!run_testcases_exp){
        return std::unexpected(run_testcases_exp.error());
    }

    return run_batch_value;
}
