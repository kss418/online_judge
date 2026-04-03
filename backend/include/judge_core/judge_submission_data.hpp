#pragma once

#include "common/submission_status.hpp"
#include "dto/submission_dto.hpp"
#include "judge_core/execution_report.hpp"
#include "judge_core/judge_result.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace judge_submission_data{
    struct process_submission_data{
        judge_result judge_result_value = judge_result::wrong_answer;
        execution_report::batch execution_report_value;
        std::int64_t prepare_workspace_elapsed_ms = 0;
        std::int64_t testcase_snapshot_elapsed_ms = 0;
    };

    struct submission_stage_metrics{
        std::string event = "completed";
        std::optional<std::string> error_message_opt = std::nullopt;
        std::int64_t queue_wait_ms = 0;
        std::int64_t prepare_workspace_elapsed_ms = 0;
        std::int64_t testcase_snapshot_elapsed_ms = 0;
        std::int64_t compile_prepare_elapsed_ms = 0;
        std::int64_t testcase_execution_elapsed_ms = 0;
        std::int64_t finalize_elapsed_ms = 0;
        std::int64_t cleanup_elapsed_ms = 0;
        std::int64_t total_elapsed_ms = 0;
        std::int32_t testcase_count = 0;
        submission_status final_submission_status = submission_status::queued;
    };

    submission_stage_metrics make_submission_stage_metrics(
        const submission_dto::queued_submission& queued_submission_value
    );

    submission_stage_metrics make_submission_stage_metrics(
        submission_stage_metrics submission_stage_metrics_value,
        const process_submission_data& process_submission_data_value
    );

    submission_stage_metrics make_requeued_submission_stage_metrics(
        const submission_dto::queued_submission& queued_submission_value,
        std::string error_message
    );

    process_submission_data make_process_submission_data(
        judge_result judge_result_value,
        execution_report::batch&& execution_report_value
    );
}
