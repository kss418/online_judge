#include "judge_core/application/submission_executor.hpp"

#include <utility>

std::expected<submission_executor, judge_error> submission_executor::create(){
    auto program_executor_exp = program_executor::create();
    if(!program_executor_exp){
        return std::unexpected(program_executor_exp.error());
    }

    return submission_executor(std::move(*program_executor_exp));
}

submission_executor::submission_executor(
    program_executor program_executor_value
) :
    program_executor_(std::move(program_executor_value)){}

submission_executor::submission_executor(
    submission_executor&& other
) noexcept = default;

submission_executor& submission_executor::operator=(
    submission_executor&& other
) noexcept = default;

submission_executor::~submission_executor() = default;

std::expected<execution_report::batch, judge_error> submission_executor::execute(
    const runnable_program& runnable_program_value,
    const testcase_snapshot& testcase_snapshot_value
){
    auto execution_report_exp = program_executor_.run(
        runnable_program_value,
        testcase_snapshot_value
    );
    if(!execution_report_exp){
        return std::unexpected(execution_report_exp.error());
    }

    return std::move(*execution_report_exp);
}
