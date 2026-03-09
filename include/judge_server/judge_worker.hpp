#pragma once

#include "common/error_code.hpp"
#include "db/submission_service.hpp"
#include "judge_server/sandbox_runner.hpp"

#include <chrono>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

class judge_worker{
public:
    static std::expected<judge_worker, error_code> create(submission_service submission_service);

    std::expected<void, error_code> run();
    std::expected<std::optional<queued_submission>, error_code> save_source_code();
    std::expected<sandbox_runner::run_result, error_code> run_one_testcase(
        const std::filesystem::path& source_file_path,
        const std::filesystem::path& input_path
    );
    std::expected<std::vector<sandbox_runner::run_result>, error_code> run_all_testcases(
        const std::filesystem::path& source_file_path,
        std::int64_t problem_id
    );

private:
    explicit judge_worker(submission_service submission_service);

    static bool is_queue_empty_error(const error_code& code);
    std::expected<std::filesystem::path, error_code> make_input_path(
        std::int64_t problem_id,
        std::int32_t order
    );
    std::expected<std::filesystem::path, error_code> make_output_path(
        std::int64_t problem_id,
        std::int32_t order
    );
    std::expected<sandbox_runner::run_result, error_code> run_cpp(
        const std::filesystem::path& source_file_path,
        const std::filesystem::path& input_path
    );
    std::expected<sandbox_runner::run_result, error_code> run_python(
        const std::filesystem::path& source_file_path,
        const std::filesystem::path& input_path
    );
    std::expected<sandbox_runner::run_result, error_code> run_java(
        const std::filesystem::path& source_file_path,
        const std::filesystem::path& input_path
    );
    static constexpr std::chrono::milliseconds notification_wait_timeout_{30000};
    static constexpr std::chrono::milliseconds source_run_time_limit_{2000};
    static constexpr std::int64_t source_run_memory_limit_mb_{256};

    submission_service submission_service_;
    std::string cpp_compiler_path_;
    std::string python_path_;
    std::string java_runtime_path_;
};
