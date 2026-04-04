#include "judge_core/application/judge_evaluator.hpp"

#include "judge_core/policy/judge_expectation_loader.hpp"
#include "judge_core/policy/judge_policy.hpp"

std::expected<judge_evaluator, judge_error> judge_evaluator::create(){
    return judge_evaluator{};
}

judge_evaluator::judge_evaluator(
    judge_evaluator&& other
) noexcept = default;

judge_evaluator& judge_evaluator::operator=(
    judge_evaluator&& other
) noexcept = default;

judge_evaluator::~judge_evaluator() = default;

std::expected<judge_result, judge_error> judge_evaluator::evaluate(
    const testcase_snapshot& testcase_snapshot_value,
    const execution_report::batch& execution_report_value
){
    const auto judge_expectation_exp = judge_expectation_loader::load(
        testcase_snapshot_value
    );
    if(!judge_expectation_exp){
        return std::unexpected(judge_error{judge_expectation_exp.error()});
    }

    return judge_policy::check_result(
        *judge_expectation_exp,
        execution_report_value
    );
}
