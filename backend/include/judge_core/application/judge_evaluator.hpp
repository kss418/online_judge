#pragma once

#include "error/judge_error.hpp"
#include "judge_core/application/build_bundle.hpp"
#include "judge_core/application/submission_decision.hpp"
#include "judge_core/testcase_snapshot/testcase_snapshot.hpp"
#include "judge_core/types/compile_failure.hpp"
#include "judge_core/types/execution_report.hpp"

#include <expected>

class judge_evaluator{
public:
    static std::expected<judge_evaluator, judge_error> create();

    judge_evaluator(judge_evaluator&& other) noexcept;
    judge_evaluator& operator=(judge_evaluator&& other) noexcept;
    ~judge_evaluator();

    judge_evaluator(const judge_evaluator&) = delete;
    judge_evaluator& operator=(const judge_evaluator&) = delete;

    submission_decision evaluate_compile_failure(
        const compile_failure& compile_failure_value
    );
    std::expected<submission_decision, judge_error> evaluate_execution(
        const testcase_snapshot& testcase_snapshot_value,
        execution_report::batch execution_report_value
    );

private:
    judge_evaluator() = default;
};
