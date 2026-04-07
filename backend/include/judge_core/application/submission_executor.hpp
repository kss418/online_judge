#pragma once

#include "error/judge_error.hpp"
#include "judge_core/application/execution_bundle.hpp"
#include "judge_core/application/workspace_session.hpp"
#include "judge_core/infrastructure/program_executor.hpp"
#include "judge_core/testcase_snapshot/testcase_snapshot.hpp"
#include "judge_core/types/runnable_program.hpp"

#include <expected>

class submission_executor{
public:
    struct execution_input{
        const runnable_program& runnable_program_value;
        const testcase_snapshot& testcase_snapshot_value;
        const workspace_session& workspace_session_value;
    };

    static std::expected<submission_executor, judge_error> create();

    explicit submission_executor(program_executor program_executor_value);

    submission_executor(submission_executor&& other) noexcept;
    submission_executor& operator=(submission_executor&& other) noexcept;
    ~submission_executor();

    submission_executor(const submission_executor&) = delete;
    submission_executor& operator=(const submission_executor&) = delete;

    std::expected<execution_bundle, judge_error> execute(
        const execution_input& execution_input_value
    );

private:
    program_executor program_executor_;
};
