#include "judge_core/application/judge_evaluator.hpp"

#include <utility>

std::expected<judge_evaluator, judge_error> judge_evaluator::create(){
    return judge_evaluator{
        expectation_reader{},
        compile_failure_translator{},
        judge_policy{}
    };
}

judge_evaluator::judge_evaluator(
    expectation_reader expectation_reader_value,
    compile_failure_translator compile_failure_translator_value,
    judge_policy judge_policy_value
) :
    expectation_reader_(std::move(expectation_reader_value)),
    compile_failure_translator_(std::move(compile_failure_translator_value)),
    judge_policy_(std::move(judge_policy_value))
{}

judge_evaluator::judge_evaluator(
    judge_evaluator&& other
) noexcept = default;

judge_evaluator& judge_evaluator::operator=(
    judge_evaluator&& other
) noexcept = default;

judge_evaluator::~judge_evaluator() = default;

submission_decision judge_evaluator::evaluate_compile_failure(
    const compile_failure& compile_failure_value
){
    submission_decision submission_decision_value;
    submission_decision_value.judge_result_value = judge_result::compile_error;
    submission_decision_value.execution_report_value =
        compile_failure_translator_.translate(
            compile_failure_value
        );
    return submission_decision_value;
}

std::expected<submission_decision, judge_error> judge_evaluator::evaluate_execution(
    const testcase_snapshot& testcase_snapshot_value,
    execution_report::batch execution_report_value
){
    const auto judge_expectation_exp = expectation_reader_.read(
        testcase_snapshot_value
    );
    if(!judge_expectation_exp){
        return std::unexpected(judge_expectation_exp.error());
    }

    const auto judge_result_exp = judge_policy_.check_result(
        *judge_expectation_exp,
        execution_report_value
    );
    if(!judge_result_exp){
        return std::unexpected(judge_result_exp.error());
    }

    submission_decision submission_decision_value;
    submission_decision_value.judge_result_value = *judge_result_exp;
    submission_decision_value.execution_report_value = std::move(execution_report_value);
    return submission_decision_value;
}
