#pragma once

#include "common/db_connection.hpp"
#include "dto/submission_dto.hpp"
#include "error/judge_error.hpp"
#include "judge_core/judge_submission_data.hpp"
#include "judge_core/testcase_snapshot_service.hpp"

#include <expected>
#include <filesystem>

class submission_execution_service{
public:
    explicit submission_execution_service(
        testcase_snapshot_service testcase_snapshot_service
    );

    std::expected<judge_submission_data::process_submission_data, judge_error>
    process_submission(
        const submission_dto::queued_submission& queued_submission_value,
        const std::filesystem::path& workspace_path,
        db_connection& testcase_snapshot_connection
    );

private:
    std::expected<std::filesystem::path, judge_error> prepare_workspace(
        const submission_dto::queued_submission& queued_submission_value,
        const std::filesystem::path& workspace_path
    );

    testcase_snapshot_service testcase_snapshot_service_;
};
