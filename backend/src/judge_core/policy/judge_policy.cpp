#include "judge_core/policy/judge_policy.hpp"

#include "judge_core/policy/checker.hpp"

#include <csignal>
#include <string_view>
#include <vector>

namespace{
    bool contains_bad_alloc(std::string_view stderr_text){
        return stderr_text.find("bad_alloc") != std::string::npos;
    }

    bool exceeded_time_limit(
        const execution_report::testcase_execution& testcase_execution_value,
        const execution_report::applied_limits& limits_value
    ){
        if(limits_value.time_limit_ms <= 0){
            return false;
        }

        return
            testcase_execution_value.killed_by_wall_clock ||
            testcase_execution_value.termination_signal == SIGXCPU ||
            testcase_execution_value.wall_time_ms > limits_value.time_limit_ms ||
            testcase_execution_value.cpu_time_ms > limits_value.time_limit_ms;
    }

    bool exceeded_output_limit(
        const execution_report::testcase_execution& testcase_execution_value,
        const execution_report::applied_limits& limits_value
    ){
        if(limits_value.output_limit_bytes <= 0){
            return false;
        }

        return
            testcase_execution_value.stdout_bytes > limits_value.output_limit_bytes ||
            testcase_execution_value.stderr_bytes > limits_value.output_limit_bytes
#ifdef SIGXFSZ
            || testcase_execution_value.termination_signal == SIGXFSZ
#endif
            ;
    }

    bool exceeded_memory_limit(
        const execution_report::testcase_execution& testcase_execution_value,
        const execution_report::applied_limits& limits_value
    ){
        if(
            limits_value.memory_limit_kb > 0 &&
            testcase_execution_value.max_rss_kb > limits_value.memory_limit_kb
        ){
            return true;
        }

        if(testcase_execution_value.termination_signal == SIGSEGV){
            return true;
        }

        if(
            testcase_execution_value.exit_code != 0 &&
            contains_bad_alloc(testcase_execution_value.stderr_text)
        ){
            return true;
        }

        return false;
    }
}

std::expected<judge_result, judge_error> judge_policy::check_result(
    const judge_expectation& judge_expectation_value,
    const execution_report::batch& execution_report_value
) const{
    std::vector<std::string> output_texts;
    output_texts.reserve(execution_report_value.executions.size());

    if(execution_report_value.compile_failed){
        return judge_result::compile_error;
    }

    for(const auto& testcase_execution_value : execution_report_value.executions){
        if(
            exceeded_time_limit(
                testcase_execution_value,
                execution_report_value.limits
            )
        ){
            return judge_result::time_limit_exceeded;
        }
        if(
            exceeded_output_limit(
                testcase_execution_value,
                execution_report_value.limits
            )
        ){
            return judge_result::output_exceeded;
        }
        if(
            exceeded_memory_limit(
                testcase_execution_value,
                execution_report_value.limits
            )
        ){
            return judge_result::memory_limit_exceeded;
        }
        if(testcase_execution_value.exit_code != 0){
            return judge_result::runtime_error;
        }

        output_texts.push_back(testcase_execution_value.stdout_text);
    }

    return checker::check_all(output_texts, judge_expectation_value);
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
