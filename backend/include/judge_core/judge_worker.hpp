#pragma once

#include "common/db_connection.hpp"
#include "common/submission_status.hpp"
#include "dto/submission_dto.hpp"
#include "error/judge_error.hpp"
#include "judge_core/execution_report.hpp"
#include "judge_core/judge_policy.hpp"
#include "judge_core/judge_submission_data.hpp"
#include "judge_core/problem_lock_registry.hpp"
#include "judge_core/submission_queue_source.hpp"
#include "judge_core/testcase_snapshot.hpp"
#include "judge_core/testcase_snapshot_service.hpp"
#include "judge_core/testcase_runner.hpp"

#include <chrono>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <memory>
#include <string>

class judge_worker{
public:
    static std::expected<judge_worker, judge_error> create(
        std::shared_ptr<problem_lock_registry> problem_lock_registry
    );

    std::expected<void, judge_error> run();
private:
    judge_worker(
        submission_queue_source submission_queue_source,
        db_connection submission_db_connection,
        db_connection testcase_snapshot_connection,
        testcase_snapshot_service testcase_snapshot_service,
        std::filesystem::path source_root_path
    );
    std::expected<void, judge_error> finalize_submission(
        std::int64_t submission_id,
        judge_result result,
        const execution_report::batch& execution_report_value
    );
    std::expected<judge_submission_data::submission_stage_metrics, judge_error> process_submission(
        const submission_dto::queued_submission& queued_submission_value,
        const std::filesystem::path& workspace_path
    );
    std::expected<void, judge_error> requeue_submission(
        std::int64_t submission_id,
        std::string reason
    );
    std::expected<judge_submission_data::process_submission_data, judge_error> judge_submission(
        const std::filesystem::path& source_file_path,
        const testcase_snapshot& testcase_snapshot_value
    );
    static constexpr std::chrono::seconds LEASE_DURATION{300};
    static constexpr std::chrono::milliseconds NOTIFICATION_WAIT_TIMEOUT{30000};

    submission_queue_source submission_queue_source_;
    db_connection db_connection_;
    db_connection testcase_snapshot_connection_;
    testcase_snapshot_service testcase_snapshot_service_;
    std::filesystem::path source_root_path_;
};
