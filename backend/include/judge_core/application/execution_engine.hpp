#pragma once

#include "error/judge_error.hpp"
#include "judge_core/gateway/testcase_snapshot_port.hpp"
#include "judge_core/infrastructure/program_builder.hpp"
#include "judge_core/infrastructure/program_runner.hpp"
#include "judge_core/types/compile_failure.hpp"
#include "judge_core/types/execution_report.hpp"
#include "judge_core/types/runnable_program.hpp"
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

    using build_result = std::variant<runnable_program, compile_failure>;

    static std::expected<execution_engine, judge_error> create(
        testcase_snapshot_port testcase_snapshot_port_value
    );

    execution_engine(execution_engine&& other) noexcept;
    execution_engine& operator=(execution_engine&& other) noexcept;
    ~execution_engine();

    execution_engine(const execution_engine&) = delete;
    execution_engine& operator=(const execution_engine&) = delete;

    std::expected<build_result, judge_error> build(
        const std::filesystem::path& source_file_path
    );
    std::expected<execution_result, judge_error> run(
        std::int64_t problem_id,
        const runnable_program& runnable_program_value
    );

private:
    execution_engine(
        program_builder program_builder_value,
        program_runner program_runner_value,
        testcase_snapshot_port testcase_snapshot_port_value
    );

    program_builder program_builder_;
    program_runner program_runner_;
    testcase_snapshot_port testcase_snapshot_port_;
};
