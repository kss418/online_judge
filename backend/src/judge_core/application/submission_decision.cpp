#include "judge_core/application/submission_decision.hpp"

#include "judge_core/policy/judge_policy.hpp"

#include <algorithm>
#include <optional>
#include <string>

namespace{
    std::optional<std::string> select_internal_message(
        const submission_verdict_summary& verdict_summary_value
    ){
        return verdict_summary_value.internal_message_opt;
    }

    std::optional<std::string> select_compile_output(
        const submission_verdict_summary& verdict_summary_value,
        submission_status submission_status_value,
        const execution_report::batch& execution_report_value
    ){
        if(submission_status_value != submission_status::compile_error){
            return std::nullopt;
        }

        if(verdict_summary_value.internal_message_opt.has_value()){
            return verdict_summary_value.internal_message_opt;
        }

        if(execution_report_value.executions.empty()){
            return std::nullopt;
        }

        const std::string& stderr_text =
            execution_report_value.executions.front().stderr_text;
        if(stderr_text.empty()){
            return std::nullopt;
        }

        return stderr_text;
    }

    std::optional<std::string> select_judge_output(
        const submission_verdict_summary& verdict_summary_value,
        submission_status submission_status_value,
        const execution_report::batch& execution_report_value
    ){
        if(submission_status_value == submission_status::compile_error){
            return std::nullopt;
        }

        if(
            submission_status_value == submission_status::runtime_error ||
            submission_status_value == submission_status::time_limit_exceeded ||
            submission_status_value == submission_status::memory_limit_exceeded ||
            submission_status_value == submission_status::output_exceeded
        ){
            if(auto internal_message_opt = select_internal_message(verdict_summary_value);
               internal_message_opt.has_value()){
                return internal_message_opt;
            }

            if(
                verdict_summary_value.first_failed_case_index_opt.has_value() &&
                *verdict_summary_value.first_failed_case_index_opt > 0
            ){
                const auto failed_index = static_cast<std::size_t>(
                    *verdict_summary_value.first_failed_case_index_opt - 1
                );
                if(failed_index < execution_report_value.executions.size()){
                    const auto& stderr_text =
                        execution_report_value.executions[failed_index].stderr_text;
                    if(!stderr_text.empty()){
                        return stderr_text;
                    }
                }
            }
        }

        return std::nullopt;
    }

    std::optional<std::int16_t> calculate_score(
        const submission_verdict_summary& verdict_summary_value,
        submission_status submission_status_value
    ){
        if(verdict_summary_value.score_opt.has_value()){
            return verdict_summary_value.score_opt;
        }

        return submission_status_value == submission_status::accepted
            ? std::optional<std::int16_t>{100}
            : std::optional<std::int16_t>{0};
    }

    std::optional<std::int64_t> calculate_elapsed_ms(
        submission_status submission_status_value,
        const execution_report::batch& execution_report_value
    ){
        if(
            submission_status_value == submission_status::compile_error ||
            execution_report_value.executions.empty()
        ){
            return std::nullopt;
        }

        std::int64_t max_elapsed_ms = 0;
        for(const auto& testcase_execution_value : execution_report_value.executions){
            max_elapsed_ms = std::max(
                max_elapsed_ms,
                testcase_execution_value.wall_time_ms
            );
        }

        return max_elapsed_ms;
    }

    std::optional<std::int64_t> calculate_max_rss_kb(
        submission_status submission_status_value,
        const execution_report::batch& execution_report_value
    ){
        if(
            submission_status_value == submission_status::compile_error ||
            execution_report_value.executions.empty()
        ){
            return std::nullopt;
        }

        std::int64_t max_rss_kb = 0;
        for(const auto& testcase_execution_value : execution_report_value.executions){
            max_rss_kb = std::max(
                max_rss_kb,
                testcase_execution_value.max_rss_kb
            );
        }

        return max_rss_kb;
    }
}

submission_dto::finalize_request submission_decision::to_finalize_request(
    const submission_dto::leased_submission& leased_submission_value
) const{
    const submission_status submission_status_value =
        judge_policy::to_submission_status(verdict_summary.overall_verdict);

    return submission_dto::make_finalize_request(
        leased_submission_value,
        submission_status_value,
        calculate_score(verdict_summary, submission_status_value),
        select_compile_output(
            verdict_summary,
            submission_status_value,
            execution_report_value
        ),
        select_judge_output(
            verdict_summary,
            submission_status_value,
            execution_report_value
        ),
        calculate_elapsed_ms(submission_status_value, execution_report_value),
        calculate_max_rss_kb(submission_status_value, execution_report_value)
    );
}
