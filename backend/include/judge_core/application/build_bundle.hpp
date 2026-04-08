#pragma once

#include "judge_core/application/submission_decision.hpp"
#include "judge_core/types/compile_failure.hpp"
#include "judge_core/types/runnable_program.hpp"

#include <cstdint>
#include <utility>
#include <variant>

class build_bundle{
public:
    static build_bundle make_runnable(runnable_program runnable_program_value){
        return build_bundle(std::move(runnable_program_value));
    }

    static build_bundle make_compile_failure(compile_failure compile_failure_value){
        return build_bundle(std::move(compile_failure_value));
    }

    bool success() const noexcept{
        return std::holds_alternative<runnable_program>(storage_);
    }

    bool compile_failed() const noexcept{
        return std::holds_alternative<compile_failure>(storage_);
    }

    const runnable_program& artifact() const{
        return std::get<runnable_program>(storage_);
    }

    const compile_failure& failure() const{
        return std::get<compile_failure>(storage_);
    }

    submission_decision to_compile_error_decision() const{
        const auto& compile_failure_value = failure();

        execution_report::testcase_execution testcase_execution_value;
        testcase_execution_value.exit_code = compile_failure_value.exit_code;
        testcase_execution_value.stderr_text = compile_failure_value.stderr_text;
        testcase_execution_value.stderr_bytes =
            static_cast<std::int64_t>(compile_failure_value.stderr_text.size());

        execution_report::batch execution_report_value;
        execution_report_value.compile_failed = true;
        execution_report_value.executions.push_back(
            std::move(testcase_execution_value)
        );

        submission_decision submission_decision_value;
        submission_decision_value.judge_result_value = judge_result::compile_error;
        submission_decision_value.execution_report_value =
            std::move(execution_report_value);
        return submission_decision_value;
    }

private:
    explicit build_bundle(runnable_program runnable_program_value) :
        storage_(std::move(runnable_program_value)){}

    explicit build_bundle(compile_failure compile_failure_value) :
        storage_(std::move(compile_failure_value)){}

    std::variant<runnable_program, compile_failure> storage_;
};
