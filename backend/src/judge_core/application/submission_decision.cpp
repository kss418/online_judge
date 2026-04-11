#include "judge_core/application/submission_decision.hpp"

#include "judge_core/policy/judge_policy.hpp"

#include <algorithm>
#include <optional>
#include <string>

namespace{
    bool is_compile_verdict(judge_result verdict_value){
        return verdict_value == judge_result::compile_error;
    }

    std::optional<std::string> select_compile_output(
        const submission_verdict_summary& verdict_summary_value
    ){
        if(!is_compile_verdict(verdict_summary_value.overall_verdict)){
            return std::nullopt;
        }

        return verdict_summary_value.internal_message_opt;
    }

    std::optional<std::string> select_judge_output(
        const submission_verdict_summary& verdict_summary_value
    ){
        if(is_compile_verdict(verdict_summary_value.overall_verdict)){
            return std::nullopt;
        }

        return verdict_summary_value.internal_message_opt;
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

submission_internal_dto::finalize_request submission_decision::to_finalize_request(
    const submission_domain_dto::leased_submission& leased_submission_value
) const{
    const submission_status submission_status_value =
        judge_policy::to_submission_status(verdict_summary.overall_verdict);

    return submission_internal_dto::make_finalize_request(
        leased_submission_value,
        submission_status_value,
        verdict_summary.score_opt,
        select_compile_output(verdict_summary),
        select_judge_output(verdict_summary),
        calculate_elapsed_ms(submission_status_value, execution_report_value),
        calculate_max_rss_kb(submission_status_value, execution_report_value)
    );
}
