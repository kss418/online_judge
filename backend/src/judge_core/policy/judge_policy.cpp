#include "judge_core/policy/judge_policy.hpp"

#include "judge_core/policy/execution_failure_classifier.hpp"
#include "judge_core/policy/checker.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace{
    std::optional<execution_report::execution_failure_kind> resolve_execution_failure(
        const execution_report::testcase_execution& testcase_execution_value,
        const execution_report::applied_limits& limits_value
    ){
        if(testcase_execution_value.failure_opt.has_value()){
            return testcase_execution_value.failure_opt;
        }

        return execution_failure_classifier::classify(
            testcase_execution_value,
            limits_value
        );
    }

    std::string make_failed_testcase_message(
        execution_report::execution_failure_kind failure_kind_value,
        std::int32_t testcase_index
    ){
        switch(failure_kind_value){
            case execution_report::execution_failure_kind::time_limit_exceeded:
                return "time limit exceeded on testcase " + std::to_string(testcase_index);
            case execution_report::execution_failure_kind::output_exceeded:
                return "output limit exceeded on testcase " + std::to_string(testcase_index);
            case execution_report::execution_failure_kind::memory_limit_exceeded:
                return "memory limit exceeded on testcase " + std::to_string(testcase_index);
            case execution_report::execution_failure_kind::runtime_error:
                return "runtime error on testcase " + std::to_string(testcase_index);
        }

        return "runtime error on testcase " + std::to_string(testcase_index);
    }

    std::optional<std::string> to_optional_message(std::string_view value){
        if(value.empty()){
            return std::nullopt;
        }

        return std::string{value};
    }

    std::optional<std::int16_t> make_score(judge_result verdict_value){
        return verdict_value == judge_result::accepted
            ? std::optional<std::int16_t>{100}
            : std::optional<std::int16_t>{0};
    }

    submission_verdict_summary make_verdict_summary(
        judge_result overall_verdict,
        std::optional<std::int32_t> first_failed_case_index_opt = std::nullopt,
        std::optional<std::string> public_message_opt = std::nullopt,
        std::optional<std::string> internal_message_opt = std::nullopt
    ){
        submission_verdict_summary summary_value;
        summary_value.overall_verdict = overall_verdict;
        summary_value.first_failed_case_index_opt = first_failed_case_index_opt;
        summary_value.public_message_opt = std::move(public_message_opt);
        summary_value.internal_message_opt = std::move(internal_message_opt);
        summary_value.score_opt = make_score(overall_verdict);
        return summary_value;
    }

    submission_verdict_summary make_failed_testcase_summary(
        judge_result overall_verdict,
        std::int32_t testcase_index,
        std::string public_message,
        std::string_view stderr_text
    ){
        return make_verdict_summary(
            overall_verdict,
            testcase_index,
            std::move(public_message),
            to_optional_message(stderr_text)
        );
    }
}

std::expected<submission_verdict_summary, judge_error> judge_policy::check_result(
    const judge_expectation& judge_expectation_value,
    const execution_report::batch& execution_report_value
) const{
    std::vector<std::string> output_texts;
    output_texts.reserve(execution_report_value.executions.size());

    if(execution_report_value.compile_failed){
        return make_verdict_summary(judge_result::compile_error);
    }

    for(std::size_t index = 0; index < execution_report_value.executions.size(); ++index){
        const auto& testcase_execution_value = execution_report_value.executions[index];
        const auto testcase_index = static_cast<std::int32_t>(index + 1);
        const auto execution_failure_opt = resolve_execution_failure(
            testcase_execution_value,
            execution_report_value.limits
        );
        if(execution_failure_opt.has_value()){
            return make_failed_testcase_summary(
                execution_failure_classifier::to_judge_result(*execution_failure_opt),
                testcase_index,
                make_failed_testcase_message(*execution_failure_opt, testcase_index),
                testcase_execution_value.stderr_text
            );
        }

        output_texts.push_back(testcase_execution_value.stdout_text);
    }

    return make_verdict_summary(
        checker::check_all(output_texts, judge_expectation_value)
    );
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
