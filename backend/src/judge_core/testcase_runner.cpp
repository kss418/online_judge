#include "judge_core/testcase_runner.hpp"

#include "common/timer.hpp"

#include <utility>
#include <vector>

namespace{
    execution_report::testcase_execution make_testcase_execution(
        const sandbox_runner::run_result& run_result_value
    ){
        execution_report::testcase_execution testcase_execution_value;
        testcase_execution_value.exit_code = run_result_value.exit_code_;
        testcase_execution_value.stdout_text = run_result_value.stdout_text_;
        testcase_execution_value.stderr_text = run_result_value.stderr_text_;
        testcase_execution_value.max_rss_kb =
            static_cast<std::int64_t>(run_result_value.max_rss_kb_);
        testcase_execution_value.elapsed_ms =
            static_cast<std::int64_t>(run_result_value.elapsed_ms_);
        testcase_execution_value.time_limit_exceeded =
            run_result_value.time_limit_exceeded_;
        testcase_execution_value.memory_limit_exceeded =
            run_result_value.memory_limit_exceeded_;
        testcase_execution_value.output_exceeded =
            run_result_value.output_exceeded_;
        return testcase_execution_value;
    }

    std::expected<execution_report::testcase_execution, judge_error> run_one_testcase(
        const std::vector<std::string>& command_args,
        const sandbox_runner::run_options& run_options_value
    ){
        const auto run_exp = sandbox_runner::run(
            command_args,
            run_options_value
        );
        if(!run_exp){
            return std::unexpected(run_exp.error());
        }

        return make_testcase_execution(*run_exp);
    }

    sandbox_runner::run_options make_base_run_options(
        const pl_runner_util::execution_plan& execution_plan_value,
        const problem_content_dto::limits& problem_limits_value
    ){
        sandbox_runner::run_options run_options_value;
        run_options_value.workspace_host_path = execution_plan_value.workspace_host_path_;
        run_options_value.time_limit =
            std::chrono::milliseconds{problem_limits_value.time_ms};
        run_options_value.memory_limit_mb = problem_limits_value.memory_mb;
        run_options_value.policy = sandbox_runner::policy_profile::run;
        run_options_value.mounts = execution_plan_value.mount_profile_;
        return run_options_value;
    }

    sandbox_runner::run_options make_testcase_run_options(
        const sandbox_runner::run_options& base_run_options_value,
        const std::filesystem::path& input_path
    ){
        auto run_options_value = base_run_options_value;
        run_options_value.input_path_opt = input_path;
        return run_options_value;
    }
}

std::expected<execution_report::batch, judge_error> testcase_runner::run_all_testcases(
    const std::filesystem::path& source_file_path,
    const testcase_snapshot& testcase_snapshot_value
){
    const auto validated_testcase_count_exp = testcase_snapshot_value.validate_testcase_layout();
    if(!validated_testcase_count_exp){
        return std::unexpected(validated_testcase_count_exp.error());
    }

    execution_report::batch execution_report_value;
    execution_report_value.testcase_count = *validated_testcase_count_exp;

    const auto build_source_exp = timer::measure_elapsed_ms(
        execution_report_value.prepare_elapsed_ms,
        [&source_file_path]{
            return pl_runner_util::instance().build_source(source_file_path);
        }
    );
    if(!build_source_exp){
        return std::unexpected(build_source_exp.error());
    }

    if(!build_source_exp->is_runnable()){
        execution_report_value.compile_failed = true;
        execution_report_value.executions.push_back(
            make_testcase_execution(
                *build_source_exp->compile_failed_run_result_
            )
        );
        return execution_report_value;
    }

    const auto execution_plan_exp =
        pl_runner_util::instance().make_execution_plan(*build_source_exp);
    if(!execution_plan_exp){
        return std::unexpected(execution_plan_exp.error());
    }

    execution_report_value.executions.reserve(
        static_cast<std::size_t>(*validated_testcase_count_exp)
    );

    const auto base_run_options_value = make_base_run_options(
        *execution_plan_exp,
        testcase_snapshot_value.limits_value
    );

    const auto run_testcases_exp = timer::measure_elapsed_ms(
        execution_report_value.testcase_execution_elapsed_ms,
        [&]() -> std::expected<void, judge_error> {
            for(std::int32_t order = 1; order <= *validated_testcase_count_exp; ++order){
                const auto input_path_exp = testcase_snapshot_value.make_input_path(order);
                if(!input_path_exp){
                    return std::unexpected(input_path_exp.error());
                }

                const auto run_options_value = make_testcase_run_options(
                    base_run_options_value,
                    *input_path_exp
                );

                const auto run_one_testcase_exp = run_one_testcase(
                    execution_plan_exp->run_command_args_,
                    run_options_value
                );
                if(!run_one_testcase_exp){
                    return std::unexpected(run_one_testcase_exp.error());
                }

                execution_report_value.executions.push_back(
                    std::move(*run_one_testcase_exp)
                );
            }

            return {};
        }
    );
    if(!run_testcases_exp){
        return std::unexpected(run_testcases_exp.error());
    }

    return execution_report_value;
}
