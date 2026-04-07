#include "judge_core/application/judge_evaluator.hpp"

#include "judge_core/application/compile_failure_report_mapper.hpp"
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

std::expected<submission_decision, judge_error> judge_evaluator::evaluate(
    const evaluation_input& evaluation_input_value
){
    if(evaluation_input_value.build_bundle_value.compile_failed()){
        submission_decision submission_decision_value;
        submission_decision_value.judge_result_value = judge_result::compile_error;
        submission_decision_value.execution_report_value =
            compile_failure_report_mapper::make_execution_report(
                evaluation_input_value.build_bundle_value.failure()
            );
        return submission_decision_value;
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

    const auto judge_result_exp = judge_policy::check_result(
        *judge_expectation_exp,
        *execution_report_value
    );
    if(!judge_result_exp){
        return std::unexpected(judge_result_exp.error());
    }

    submission_decision submission_decision_value;
    submission_decision_value.judge_result_value = *judge_result_exp;
    submission_decision_value.execution_report_value =
        std::move(*execution_report_value);
    return submission_decision_value;
}
