#pragma once

#include "common/db_connection.hpp"
#include "dto/submission_dto.hpp"
#include "error/judge_error.hpp"
#include "judge_core/application/submission_execution_service.hpp"
#include "judge_core/types/execution_report.hpp"

#include <cstdint>
#include <expected>
#include <filesystem>
#include <string>

class submission_processor{
public:
    struct dependencies{
        db_connection testcase_snapshot_connection;
        submission_execution_service submission_execution_service_value;
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

    std::expected<void, judge_error> process_submission(
        db_connection& submission_db_connection,
        const submission_dto::queued_submission& queued_submission_value
    );

private:
    submission_processor(
        db_connection testcase_snapshot_connection,
        submission_execution_service submission_execution_service,
        std::filesystem::path source_root_path
    );

    std::expected<void, judge_error> execute_submission(
        db_connection& submission_db_connection,
        const submission_dto::queued_submission& queued_submission_value,
        const std::filesystem::path& workspace_path
    );
    std::expected<void, judge_error> finalize_submission(
        db_connection& submission_db_connection,
        std::int64_t submission_id,
        judge_result result,
        const execution_report::batch& execution_report_value
    );
    std::expected<void, judge_error> requeue_submission(
        db_connection& submission_db_connection,
        std::int64_t submission_id,
        std::string reason
    );

    db_connection testcase_snapshot_connection_;
    submission_execution_service submission_execution_service_;
    std::filesystem::path source_root_path_;
};
