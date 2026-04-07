#include "judge_core/application/judge_evaluator.hpp"

#include "judge_core/policy/judge_expectation_loader.hpp"
#include "judge_core/policy/judge_policy.hpp"

#include <utility>

namespace{
    judge_error make_validation_error(const char* message){
        return judge_error{judge_error_code::validation_error, message};
    }
}

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
    const evaluation_input& evaluation_input_value
){
    if(evaluation_input_value.build_bundle_value.compile_failed()){
        return judge_result::compile_error;
    }

    if(!evaluation_input_value.build_bundle_value.success()){
        return std::unexpected(make_validation_error("missing build result"));
    }

    if(evaluation_input_value.testcase_snapshot_value_ptr == nullptr){
        return std::unexpected(make_validation_error("missing testcase snapshot"));
    }

    const auto* execution_report_value =
        evaluation_input_value.execution_bundle_value.execution_report_opt();
    if(execution_report_value == nullptr){
        return std::unexpected(make_validation_error("missing execution report"));
    }

    const auto judge_expectation_exp = judge_expectation_loader::load(
        *evaluation_input_value.testcase_snapshot_value_ptr
    );
    if(!judge_expectation_exp){
        return std::unexpected(judge_error{judge_expectation_exp.error()});
    }

    return judge_policy::check_result(
        *judge_expectation_exp,
        *execution_report_value
    );
}
