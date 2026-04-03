#pragma once

#include "common/submission_status.hpp"
#include "common/db_connection.hpp"
#include "db_event/submission_event_listener.hpp"
#include "dto/submission_dto.hpp"
#include "error/judge_error.hpp"
#include "judge_core/problem_lock_registry.hpp"
#include "judge_core/testcase_snapshot.hpp"
#include "judge_core/testcase_downloader.hpp"
#include "judge_core/testcase_runner.hpp"
#include "judge_core/judge_workspace.hpp"
#include "judge_core/sandbox_runner.hpp"

#include <chrono>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class judge_worker{
public:
    static std::expected<judge_worker, judge_error> create(
        submission_event_listener submission_event_listener,
        std::shared_ptr<problem_lock_registry> problem_lock_registry
    );

    std::expected<void, judge_error> run();

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
private:
    struct finalize_submission_data{
        std::optional<std::int16_t> score = std::nullopt;
        std::optional<std::string> compile_output = std::nullopt;
        std::optional<std::string> judge_output = std::nullopt;
        std::optional<std::int64_t> elapsed_ms_opt = std::nullopt;
        std::optional<std::int64_t> max_rss_kb_opt = std::nullopt;
    };
    struct process_submission_data{
        judge_result judge_result_value = judge_result::wrong_answer;
        std::vector<sandbox_runner::run_result> run_results;
        std::int32_t testcase_count = 0;
        std::int64_t compile_prepare_elapsed_ms = 0;
        std::int64_t testcase_execution_elapsed_ms = 0;
    };

    judge_worker(
        submission_event_listener submission_event_listener,
        db_connection submission_db_connection,
        db_connection testcase_downloader_connection,
        std::filesystem::path source_root_path,
        std::shared_ptr<problem_lock_registry> problem_lock_registry
    );

    static submission_status to_submission_status(judge_result result);
    static finalize_submission_data make_finalize_submission_data(
        submission_status submission_status_value,
        const std::vector<sandbox_runner::run_result>& run_results
    );
    static submission_stage_metrics make_submission_stage_metrics(
        const submission_dto::queued_submission& queued_submission_value
    );
    static submission_stage_metrics make_submission_stage_metrics(
        submission_stage_metrics submission_stage_metrics_value,
        const process_submission_data& process_submission_data_value
    );
    static submission_stage_metrics make_requeued_submission_stage_metrics(
        const submission_dto::queued_submission& queued_submission_value,
        std::string error_message
    );
    static process_submission_data make_process_submission_data(
        judge_result judge_result_value,
        testcase_runner::run_batch&& run_batch_value
    );
    std::expected<void, judge_error> finalize_submission(
        std::int64_t submission_id,
        judge_result result,
        const std::vector<sandbox_runner::run_result>& run_results
    );
    std::expected<submission_stage_metrics, judge_error> process_submission(
        const submission_dto::queued_submission& queued_submission_value,
        const std::filesystem::path& workspace_path
    );
    std::expected<void, judge_error> requeue_submission(
        std::int64_t submission_id,
        std::string reason
    );
    std::expected<process_submission_data, judge_error> judge_submission(
        const std::filesystem::path& source_file_path,
        const testcase_snapshot& testcase_snapshot_value
    );
    std::expected<judge_result, judge_error> check_result(
        const testcase_snapshot& testcase_snapshot_value,
        const testcase_runner::run_batch& run_batch_value
    );
    static constexpr std::chrono::seconds LEASE_DURATION{300};
    static constexpr std::chrono::milliseconds NOTIFICATION_WAIT_TIMEOUT{30000};

    submission_event_listener submission_event_listener_;
    db_connection db_connection_;
    db_connection testcase_downloader_connection_;
    std::filesystem::path source_root_path_;
    std::shared_ptr<problem_lock_registry> problem_lock_registry_;
};
