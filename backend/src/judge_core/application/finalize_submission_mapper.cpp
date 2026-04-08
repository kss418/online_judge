#include "judge_core/application/finalize_submission_mapper.hpp"

#include "judge_core/policy/judge_policy.hpp"

#include <algorithm>
#include <optional>
#include <string>

namespace{
    std::optional<std::string> select_compile_output(
        submission_status submission_status_value,
        const execution_report::batch& execution_report_value
    ){
        if(
            submission_status_value != submission_status::compile_error ||
            execution_report_value.executions.empty()
        ){
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
        submission_status submission_status_value,
        const execution_report::batch& execution_report_value
    ){
        if(
            execution_report_value.executions.empty() ||
            submission_status_value == submission_status::compile_error
        ){
            return std::nullopt;
        }

        const std::string& stderr_text =
            execution_report_value.executions.front().stderr_text;
        if(stderr_text.empty()){
            return std::nullopt;
        }

        if(
            submission_status_value == submission_status::runtime_error ||
            submission_status_value == submission_status::time_limit_exceeded ||
            submission_status_value == submission_status::memory_limit_exceeded ||
            submission_status_value == submission_status::output_exceeded
        ){
            return stderr_text;
        }

        return std::nullopt;
    }

    std::optional<std::int16_t> calculate_score(
        submission_status submission_status_value
    ){
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

submission_dto::finalize_request finalize_submission_mapper::make_finalize_request(
    std::int64_t submission_id,
    judge_result result,
    const execution_report::batch& execution_report_value
){
    const submission_status submission_status_value =
        judge_policy::to_submission_status(result);

    return submission_dto::make_finalize_request(
        submission_id,
        submission_status_value,
        calculate_score(submission_status_value),
        select_compile_output(submission_status_value, execution_report_value),
        select_judge_output(submission_status_value, execution_report_value),
        calculate_elapsed_ms(submission_status_value, execution_report_value),
        calculate_max_rss_kb(submission_status_value, execution_report_value)
    );
}
