#pragma once

#include "common/db_connection.hpp"
#include "dto/submission_dto.hpp"
#include "error/judge_error.hpp"
#include "judge_core/application/submission_execution_service.hpp"
#include "judge_core/infrastructure/submission_queue_source.hpp"
#include "judge_core/policy/judge_policy.hpp"
#include "judge_core/types/execution_report.hpp"

#include <chrono>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <string>

class judge_worker{
public:
    struct dependencies{
        submission_queue_source submission_queue_source_value;
        db_connection submission_db_connection;
        db_connection testcase_snapshot_connection;
        submission_execution_service submission_execution_service_value;
        std::filesystem::path source_root_path;
    };

    static std::expected<judge_worker, judge_error> create(
        dependencies dependencies_value
    );

    std::expected<void, judge_error> run();
private:
    judge_worker(
        submission_queue_source submission_queue_source,
        db_connection submission_db_connection,
        db_connection testcase_snapshot_connection,
        submission_execution_service submission_execution_service,
        std::filesystem::path source_root_path
    );
    std::expected<void, judge_error> finalize_submission(
        std::int64_t submission_id,
        judge_result result,
        const execution_report::batch& execution_report_value
    );
    std::expected<void, judge_error> process_submission(
        const submission_dto::queued_submission& queued_submission_value,
        const std::filesystem::path& workspace_path
    );
    std::expected<void, judge_error> requeue_submission(
        std::int64_t submission_id,
        std::string reason
    );
    static constexpr std::chrono::seconds LEASE_DURATION{300};
    static constexpr std::chrono::milliseconds NOTIFICATION_WAIT_TIMEOUT{30000};

    submission_queue_source submission_queue_source_;
    db_connection db_connection_;
    db_connection testcase_snapshot_connection_;
    submission_execution_service submission_execution_service_;
    std::filesystem::path source_root_path_;
};
