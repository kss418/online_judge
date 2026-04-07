#pragma once

#include "error/judge_error.hpp"
#include "judge_core/application/build_bundle.hpp"
#include "judge_core/application/execution_bundle.hpp"
#include "judge_core/testcase_snapshot/testcase_snapshot.hpp"
#include "judge_core/types/judge_result.hpp"

#include <expected>

class judge_evaluator{
public:
    struct evaluation_input{
        const build_bundle& build_bundle_value;
        const execution_bundle& execution_bundle_value;
        const testcase_snapshot* testcase_snapshot_value_ptr = nullptr;
    };

    static std::expected<judge_evaluator, judge_error> create();

    judge_evaluator(judge_evaluator&& other) noexcept;
    judge_evaluator& operator=(judge_evaluator&& other) noexcept;
    ~judge_evaluator();

    judge_evaluator(const judge_evaluator&) = delete;
    judge_evaluator& operator=(const judge_evaluator&) = delete;

    std::expected<judge_result, judge_error> evaluate(
        const evaluation_input& evaluation_input_value
    );

private:
    judge_evaluator() = default;
};
