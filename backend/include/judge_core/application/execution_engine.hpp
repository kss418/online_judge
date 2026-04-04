#pragma once

#include "dto/submission_dto.hpp"
#include "error/judge_error.hpp"
#include "judge_core/gateway/testcase_snapshot_port.hpp"
#include "judge_core/types/execution_report.hpp"
#include "judge_core/types/judge_submission_data.hpp"
#include "judge_core/types/testcase_snapshot.hpp"

#include <expected>
#include <filesystem>
#include <memory>
#include <variant>

class program_builder;
class launch_planner;

class execution_engine{
public:
    struct execution_result{
        testcase_snapshot testcase_snapshot_value;
        execution_report::batch execution_report_value;
    };

    using execute_result =
        std::variant<execution_result, judge_submission_data::process_submission_data>;

    static std::expected<execution_engine, judge_error> create(
        testcase_snapshot_port testcase_snapshot_port_value
    );

    execution_engine(execution_engine&& other) noexcept;
    execution_engine& operator=(execution_engine&& other) noexcept;
    ~execution_engine();

    execution_engine(const execution_engine&) = delete;
    execution_engine& operator=(const execution_engine&) = delete;

    std::expected<execute_result, judge_error> execute(
        const submission_dto::queued_submission& queued_submission_value,
        const std::filesystem::path& workspace_path
    );

private:
    std::expected<std::filesystem::path, judge_error> write_submission_source(
        const submission_dto::queued_submission& queued_submission_value,
        const std::filesystem::path& workspace_path
    );

    execution_engine(
        std::unique_ptr<program_builder> program_builder_value,
        std::unique_ptr<launch_planner> launch_planner_value,
        testcase_snapshot_port testcase_snapshot_port_value
    );

    std::unique_ptr<program_builder> program_builder_;
    std::unique_ptr<launch_planner> launch_planner_;
    testcase_snapshot_port testcase_snapshot_port_;
};
