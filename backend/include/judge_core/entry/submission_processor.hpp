#pragma once

#include "dto/submission_dto.hpp"
#include "error/judge_error.hpp"
#include "judge_core/application/execution_engine.hpp"
#include "judge_core/application/judge_evaluator.hpp"
#include "judge_core/application/workspace_runner.hpp"
#include "judge_core/gateway/judge_queue_facade.hpp"
#include "judge_core/gateway/judge_submission_facade.hpp"
#include "judge_core/types/execution_report.hpp"
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
        execution_engine execution_engine_value;
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
        judge_submission_facade judge_submission_facade_value,
        execution_engine execution_engine_value,
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
    std::expected<void, judge_error> finalize_submission(
        std::int64_t submission_id,
        judge_result result,
        const execution_report::batch& execution_report_value
    );
    std::expected<void, judge_error> requeue_submission(
        std::int64_t submission_id,
        std::string reason
    );

    judge_queue_facade judge_queue_facade_;
    judge_submission_facade judge_submission_facade_;
    execution_engine execution_engine_;
    judge_evaluator judge_evaluator_;
    workspace_runner workspace_runner_;
};
