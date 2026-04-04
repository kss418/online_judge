#pragma once

#include "dto/submission_dto.hpp"
#include "error/judge_error.hpp"
#include "judge_core/gateway/testcase_snapshot_port.hpp"
#include "judge_core/types/judge_submission_data.hpp"

#include <expected>
#include <filesystem>
#include <memory>

class program_builder;
class launch_planner;
struct toolchain_config;

class submission_execution_service{
public:
    static std::expected<submission_execution_service, judge_error> create(
        const toolchain_config& toolchain_config_value,
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
    std::expected<std::filesystem::path, judge_error> prepare_workspace(
        const submission_dto::queued_submission& queued_submission_value,
        const std::filesystem::path& workspace_path
    );

    submission_execution_service(
        std::unique_ptr<program_builder> program_builder_value,
        std::unique_ptr<launch_planner> launch_planner_value,
        testcase_snapshot_port testcase_snapshot_port_value
    );

    std::unique_ptr<program_builder> program_builder_;
    std::unique_ptr<launch_planner> launch_planner_;
    testcase_snapshot_port testcase_snapshot_port_;
};
