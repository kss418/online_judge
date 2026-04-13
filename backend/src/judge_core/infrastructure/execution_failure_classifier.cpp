#include "judge_core/infrastructure/execution_failure_classifier.hpp"

#include <csignal>
#include <string_view>

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

std::optional<execution_report::execution_failure_kind> execution_failure_classifier::classify(
    const execution_report::testcase_execution& testcase_execution_value,
    const execution_report::applied_limits& limits_value
){
    if(exceeded_time_limit(testcase_execution_value, limits_value)){
        return execution_report::execution_failure_kind::time_limit_exceeded;
    }

    if(exceeded_output_limit(testcase_execution_value, limits_value)){
        return execution_report::execution_failure_kind::output_exceeded;
    }

    if(exceeded_memory_limit(testcase_execution_value, limits_value)){
        return execution_report::execution_failure_kind::memory_limit_exceeded;
    }

    if(testcase_execution_value.exit_code != 0){
        return execution_report::execution_failure_kind::runtime_error;
    }

    return std::nullopt;
}
