#include "judge_core/judge_policy.hpp"

#include "judge_core/checker.hpp"

#include <algorithm>
#include <vector>

std::expected<judge_result, judge_error> judge_policy::check_result(
    const testcase_snapshot& testcase_snapshot_value,
    const testcase_runner::run_batch& run_batch_value
){
    std::vector<std::string> output_texts;
    output_texts.reserve(run_batch_value.run_results.size());

    if(run_batch_value.compile_failed){
        return judge_result::compile_error;
    }

    for(const auto& run_result : run_batch_value.run_results){
        if(run_result.time_limit_exceeded_){
            return judge_result::time_limit_exceeded;
        }
        if(run_result.output_exceeded_){
            return judge_result::output_exceeded;
        }
        if(run_result.memory_limit_exceeded_){
            return judge_result::memory_limit_exceeded;
        }
        if(run_result.exit_code_ != 0){
            return judge_result::runtime_error;
        }

        output_texts.push_back(run_result.stdout_text_);
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
    const std::vector<sandbox_runner::run_result>& run_results
){
    finalize_submission_data finalize_submission_data_value;
    finalize_submission_data_value.score = std::int16_t{0};

    if(submission_status_value == submission_status::accepted){
        finalize_submission_data_value.score = std::int16_t{100};
    }

    if(
        submission_status_value != submission_status::compile_error &&
        !run_results.empty()
    ){
        std::int64_t max_elapsed_ms = 0;
        std::int64_t max_rss_kb = 0;

        for(const auto& run_result : run_results){
            max_elapsed_ms = std::max(
                max_elapsed_ms,
                static_cast<std::int64_t>(run_result.elapsed_ms_)
            );
            max_rss_kb = std::max(
                max_rss_kb,
                static_cast<std::int64_t>(run_result.max_rss_kb_)
            );
        }

        finalize_submission_data_value.elapsed_ms_opt = max_elapsed_ms;
        finalize_submission_data_value.max_rss_kb_opt = max_rss_kb;
    }

    if(run_results.empty() || run_results.front().stderr_text_.empty()){
        return finalize_submission_data_value;
    }

    if(submission_status_value == submission_status::compile_error){
        finalize_submission_data_value.compile_output = run_results.front().stderr_text_;
        return finalize_submission_data_value;
    }

    if(
        submission_status_value == submission_status::runtime_error ||
        submission_status_value == submission_status::time_limit_exceeded ||
        submission_status_value == submission_status::memory_limit_exceeded ||
        submission_status_value == submission_status::output_exceeded
    ){
        finalize_submission_data_value.judge_output = run_results.front().stderr_text_;
    }

    return finalize_submission_data_value;
}
