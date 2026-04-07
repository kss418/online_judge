#pragma once

#include "dto/submission_dto.hpp"
#include "error/judge_error.hpp"
#include "judge_core/application/judge_evaluator.hpp"
#include "judge_core/application/submission_builder.hpp"
#include "judge_core/application/submission_lifecycle.hpp"
#include "judge_core/application/workspace_manager.hpp"
#include "judge_core/gateway/judge_submission_facade.hpp"
#include "judge_core/gateway/testcase_snapshot_facade.hpp"
#include "judge_core/infrastructure/program_executor.hpp"

#include <expected>
#include <filesystem>

class submission_processor{
public:
    struct dependencies{
        judge_submission_facade judge_submission_facade_value;
        testcase_snapshot_facade testcase_snapshot_facade_value;
        submission_builder submission_builder_value;
        program_executor program_executor_value;
        judge_evaluator judge_evaluator_value;
        std::filesystem::path source_root_path;
    };

    static std::expected<submission_processor, judge_error> create(
        dependencies dependencies_value
    );

    submission_processor(submission_processor&& other) noexcept;
    submission_processor& operator=(submission_processor&& other) noexcept;
    ~submission_processor();

    submission_processor(const submission_processor&) = delete;
    submission_processor& operator=(const submission_processor&) = delete;

    std::expected<void, judge_error> process(
        const submission_dto::queued_submission& queued_submission_value
    );

private:
    submission_processor(
        submission_lifecycle submission_lifecycle_value,
        testcase_snapshot_facade testcase_snapshot_facade_value,
        submission_builder submission_builder_value,
        program_executor program_executor_value,
        judge_evaluator judge_evaluator_value,
        workspace_manager workspace_manager_value
    );

    submission_lifecycle submission_lifecycle_;
    testcase_snapshot_facade testcase_snapshot_facade_;
    submission_builder submission_builder_;
    program_executor program_executor_;
    judge_evaluator judge_evaluator_;
    workspace_manager workspace_manager_;
};
