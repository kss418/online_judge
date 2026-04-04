#pragma once

#include "error/judge_error.hpp"
#include "judge_core/types/execution_report.hpp"
#include "judge_core/types/judge_result.hpp"
#include "judge_core/types/testcase_snapshot.hpp"

#include <expected>

class judge_evaluator{
public:
    static std::expected<judge_evaluator, judge_error> create();

    judge_evaluator(judge_evaluator&& other) noexcept;
    judge_evaluator& operator=(judge_evaluator&& other) noexcept;
    ~judge_evaluator();

    judge_evaluator(const judge_evaluator&) = delete;
    judge_evaluator& operator=(const judge_evaluator&) = delete;

    std::expected<judge_result, judge_error> evaluate(
        const testcase_snapshot& testcase_snapshot_value,
        const execution_report::batch& execution_report_value
    );

private:
    judge_evaluator() = default;
};
