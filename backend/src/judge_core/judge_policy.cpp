#include "judge_core/judge_policy.hpp"

#include "judge_core/checker.hpp"

#include <algorithm>
#include <vector>

std::expected<judge_result, judge_error> judge_policy::check_result(
    const testcase_snapshot& testcase_snapshot_value,
    const execution_report::batch& execution_report_value
){
    std::vector<std::string> output_texts;
    output_texts.reserve(execution_report_value.executions.size());

    if(execution_report_value.compile_failed){
        return judge_result::compile_error;
    }

    for(const auto& testcase_execution_value : execution_report_value.executions){
        if(testcase_execution_value.time_limit_exceeded){
            return judge_result::time_limit_exceeded;
        }
        if(testcase_execution_value.output_exceeded){
            return judge_result::output_exceeded;
        }
        if(testcase_execution_value.memory_limit_exceeded){
            return judge_result::memory_limit_exceeded;
        }
        if(testcase_execution_value.exit_code != 0){
            return judge_result::runtime_error;
        }

        output_texts.push_back(testcase_execution_value.stdout_text);
    }

    return checker::check_all(output_texts, testcase_snapshot_value);
}

submission_status judge_policy::to_submission_status(judge_result result){
    switch(result){
        case judge_result::accepted:
            return submission_status::accepted;
        case judge_result::wrong_answer:
            return submission_status::wrong_answer;
        case judge_result::time_limit_exceeded:
            return submission_status::time_limit_exceeded;
        case judge_result::memory_limit_exceeded:
            return submission_status::memory_limit_exceeded;
        case judge_result::runtime_error:
            return submission_status::runtime_error;
        case judge_result::compile_error:
            return submission_status::compile_error;
        case judge_result::output_exceeded:
            return submission_status::output_exceeded;
        case judge_result::invalid_output:
            return submission_status::wrong_answer;
    }

    return submission_status::wrong_answer;
}

judge_policy::finalize_submission_data judge_policy::make_finalize_submission_data(
    submission_status submission_status_value,
    const execution_report::batch& execution_report_value
){
    finalize_submission_data finalize_submission_data_value;
    finalize_submission_data_value.score = std::int16_t{0};

    if(submission_status_value == submission_status::accepted){
        finalize_submission_data_value.score = std::int16_t{100};
    }

    if(
        submission_status_value != submission_status::compile_error &&
        !execution_report_value.executions.empty()
    ){
        std::int64_t max_elapsed_ms = 0;
        std::int64_t max_rss_kb = 0;

        for(const auto& testcase_execution_value : execution_report_value.executions){
            max_elapsed_ms = std::max(max_elapsed_ms, testcase_execution_value.elapsed_ms);
            max_rss_kb = std::max(max_rss_kb, testcase_execution_value.max_rss_kb);
        }

        finalize_submission_data_value.elapsed_ms_opt = max_elapsed_ms;
        finalize_submission_data_value.max_rss_kb_opt = max_rss_kb;
    }

    if(
        execution_report_value.executions.empty() ||
        execution_report_value.executions.front().stderr_text.empty()
    ){
        return finalize_submission_data_value;
    }

    if(submission_status_value == submission_status::compile_error){
        finalize_submission_data_value.compile_output =
            execution_report_value.executions.front().stderr_text;
        return finalize_submission_data_value;
    }

    if(
        submission_status_value == submission_status::runtime_error ||
        submission_status_value == submission_status::time_limit_exceeded ||
        submission_status_value == submission_status::memory_limit_exceeded ||
        submission_status_value == submission_status::output_exceeded
    ){
        finalize_submission_data_value.judge_output =
            execution_report_value.executions.front().stderr_text;
    }

    return finalize_submission_data_value;
}
