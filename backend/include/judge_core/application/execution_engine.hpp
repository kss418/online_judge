#pragma once

#include "dto/submission_dto.hpp"
#include "error/judge_error.hpp"
#include "judge_core/gateway/testcase_snapshot_port.hpp"
#include "judge_core/infrastructure/launch_planner.hpp"
#include "judge_core/infrastructure/program_build_types.hpp"
#include "judge_core/infrastructure/program_builder.hpp"
#include "judge_core/types/execution_report.hpp"
#include "judge_core/types/testcase_snapshot.hpp"

#include <cstdint>
#include <expected>
#include <filesystem>
#include <variant>

class execution_engine{
public:
    struct execution_result{
        testcase_snapshot testcase_snapshot_value;
        execution_report::batch execution_report_value;
    };

    using build_result =
        std::variant<program_build::build_artifact, program_build::compile_failure>;

    static std::expected<execution_engine, judge_error> create(
        testcase_snapshot_port testcase_snapshot_port_value
    );

    execution_engine(execution_engine&& other) noexcept;
    execution_engine& operator=(execution_engine&& other) noexcept;
    ~execution_engine();

    execution_engine(const execution_engine&) = delete;
    execution_engine& operator=(const execution_engine&) = delete;

    std::expected<build_result, judge_error> build(
        const submission_dto::queued_submission& queued_submission_value,
        const std::filesystem::path& workspace_path
    );
    std::expected<execution_result, judge_error> run(
        std::int64_t problem_id,
        const program_build::build_artifact& build_artifact_value
    );

private:
    std::expected<std::filesystem::path, judge_error> write_submission_source(
        const submission_dto::queued_submission& queued_submission_value,
        const std::filesystem::path& workspace_path
    );

    execution_engine(
        program_builder program_builder_value,
        launch_planner launch_planner_value,
        testcase_snapshot_port testcase_snapshot_port_value
    );

    program_builder program_builder_;
    launch_planner launch_planner_;
    testcase_snapshot_port testcase_snapshot_port_;
};
