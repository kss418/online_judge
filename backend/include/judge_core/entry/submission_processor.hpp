#pragma once

#include "dto/submission_dto.hpp"
#include "error/judge_error.hpp"
#include "judge_core/application/judge_evaluator.hpp"
#include "judge_core/application/snapshot_provider.hpp"
#include "judge_core/application/submission_builder.hpp"
#include "judge_core/application/submission_executor.hpp"
#include "judge_core/application/submission_lifecycle.hpp"
#include "judge_core/application/workspace_runner.hpp"
#include "judge_core/gateway/judge_queue_facade.hpp"
#include "judge_core/gateway/judge_submission_facade.hpp"
#include "judge_core/types/judge_submission_data.hpp"

#include <chrono>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <string>

class submission_processor{
public:
    struct dependencies{
        judge_queue_facade judge_queue_facade_value;
        judge_submission_facade judge_submission_facade_value;
        testcase_snapshot_facade testcase_snapshot_facade_value;
        submission_builder submission_builder_value;
        submission_executor submission_executor_value;
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

    std::expected<void, judge_error> process_next_submission(
        std::chrono::seconds lease_duration,
        std::chrono::milliseconds notification_wait_timeout
    );

private:
    submission_processor(
        judge_queue_facade judge_queue_facade_value,
        submission_lifecycle submission_lifecycle_value,
        snapshot_provider snapshot_provider_value,
        submission_builder submission_builder_value,
        submission_executor submission_executor_value,
        judge_evaluator judge_evaluator_value,
        workspace_runner workspace_runner_value
    );

    std::expected<void, judge_error> execute_submission(
        const submission_dto::queued_submission& queued_submission_value
    );
    std::expected<judge_submission_data::process_submission_data, judge_error>
    process_submission_in_workspace(
        const submission_dto::queued_submission& queued_submission_value
    );

    judge_queue_facade judge_queue_facade_;
    submission_lifecycle submission_lifecycle_;
    snapshot_provider snapshot_provider_;
    submission_builder submission_builder_;
    submission_executor submission_executor_;
    judge_evaluator judge_evaluator_;
    workspace_runner workspace_runner_;
};
