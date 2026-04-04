#pragma once

#include "judge_core/application/execution_engine.hpp"
#include "dto/submission_dto.hpp"
#include "error/judge_error.hpp"
#include "judge_core/types/judge_submission_data.hpp"

#include <expected>
#include <filesystem>

class submission_execution_service{
public:
    static std::expected<submission_execution_service, judge_error> create(
        testcase_snapshot_port testcase_snapshot_port_value
    );

    submission_execution_service(submission_execution_service&& other) noexcept;
    submission_execution_service& operator=(submission_execution_service&& other) noexcept;
    ~submission_execution_service();

    submission_execution_service(const submission_execution_service&) = delete;
    submission_execution_service& operator=(const submission_execution_service&) = delete;

    std::expected<judge_submission_data::process_submission_data, judge_error>
    process_submission(
        const submission_dto::queued_submission& queued_submission_value,
        const std::filesystem::path& workspace_path
    );

private:
    explicit submission_execution_service(execution_engine execution_engine_value);

    execution_engine execution_engine_;
};
