#include "judge_core/infrastructure/testcase_runner.hpp"

#include "judge_core/infrastructure/execution_failure_classifier.hpp"
#include "judge_core/infrastructure/nsjail_util.hpp"

#include <utility>
#include <vector>

namespace{
    bool has_recorded_failure(const execution_report::batch& execution_report_value){
        for(const auto& testcase_execution_value : execution_report_value.executions){
            if(testcase_execution_value.failure_opt.has_value()){
                return true;
            }
        }

        return false;
    }

    execution_report::testcase_execution make_testcase_execution(
        const sandbox_runner::run_result& run_result_value
    ){
        execution_report::testcase_execution testcase_execution_value;
        testcase_execution_value.exit_code = run_result_value.exit_code_;
        testcase_execution_value.termination_signal =
            run_result_value.termination_signal_;
        testcase_execution_value.killed_by_wall_clock =
            run_result_value.killed_by_wall_clock_;
        testcase_execution_value.stdout_text = run_result_value.stdout_text_;
        testcase_execution_value.stderr_text = run_result_value.stderr_text_;
        testcase_execution_value.stdout_bytes =
            static_cast<std::int64_t>(run_result_value.stdout_bytes_);
        testcase_execution_value.stderr_bytes =
            static_cast<std::int64_t>(run_result_value.stderr_bytes_);
        testcase_execution_value.max_rss_kb =
            static_cast<std::int64_t>(run_result_value.max_rss_kb_);
        testcase_execution_value.wall_time_ms =
            static_cast<std::int64_t>(run_result_value.wall_time_ms_);
        testcase_execution_value.cpu_time_ms =
            static_cast<std::int64_t>(run_result_value.cpu_time_ms_);
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
        const program_launch::execution_plan& execution_plan_value,
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

    void apply_stderr_policy(
        execution_report::testcase_execution& testcase_execution_value,
        const execution_report::batch& execution_report_value,
        const execution_policy& execution_policy_value
    ){
        if(
            execution_policy_value.stderr_policy ==
            stderr_capture_policy::keep_all
        ){
            return;
        }

        if(
            execution_policy_value.stderr_policy ==
                stderr_capture_policy::keep_first_failure_only &&
            testcase_execution_value.failure_opt.has_value() &&
            !has_recorded_failure(execution_report_value)
        ){
            return;
        }

        testcase_execution_value.stderr_text.clear();
        testcase_execution_value.stderr_bytes = 0;
    }

    bool should_stop_after(
        const execution_report::testcase_execution& testcase_execution_value,
        const execution_policy& execution_policy_value
    ){
        return
            execution_policy_value.stop_rule ==
                execution_stop_rule::stop_on_first_execution_failure &&
            testcase_execution_value.failure_opt.has_value();
    }
}

std::expected<execution_report::batch, judge_error> testcase_runner::run_testcases(
    const program_launch::execution_plan& execution_plan_value,
    const testcase_snapshot& testcase_snapshot_value,
    const execution_policy& execution_policy_value
){
    const auto validate_exp = testcase_snapshot_value.validate();
    if(!validate_exp){
        return std::unexpected(validate_exp.error());
    }

    execution_report::batch execution_report_value;
    execution_report_value.testcase_count = testcase_snapshot_value.case_count();
    execution_report_value.limits.time_limit_ms =
        testcase_snapshot_value.limits_value.time_ms;
    execution_report_value.limits.memory_limit_kb =
        static_cast<std::int64_t>(testcase_snapshot_value.limits_value.memory_mb) *
        1024LL;
    execution_report_value.limits.output_limit_bytes =
        nsjail_util::output_file_limit_bytes();

    execution_report_value.executions.reserve(
        static_cast<std::size_t>(testcase_snapshot_value.case_count())
    );

    const auto base_run_options_value = make_base_run_options(
        execution_plan_value,
        testcase_snapshot_value.limits_value
    );

    for(std::int32_t order = 1; order <= testcase_snapshot_value.case_count(); ++order){
        const auto input_path_exp = testcase_snapshot_value.input_path(order);
        if(!input_path_exp){
            return std::unexpected(input_path_exp.error());
        }

        const auto run_options_value = make_testcase_run_options(
            base_run_options_value,
            *input_path_exp
        );

        auto run_one_testcase_exp = run_one_testcase(
            execution_plan_value.run_command_args_,
            run_options_value
        );
        if(!run_one_testcase_exp){
            return std::unexpected(run_one_testcase_exp.error());
        }

        run_one_testcase_exp->failure_opt = execution_failure_classifier::classify(
            *run_one_testcase_exp,
            execution_report_value.limits
        );

        apply_stderr_policy(
            *run_one_testcase_exp,
            execution_report_value,
            execution_policy_value
        );

        execution_report_value.executions.push_back(
            std::move(*run_one_testcase_exp)
        );

        if(should_stop_after(
            execution_report_value.executions.back(),
            execution_policy_value
        )){
            break;
        }
    }

    return execution_report_value;
}
