#pragma once

#include "common/error_code.hpp"
#include "db/submission_service.hpp"
#include "judge_server/code_runner.hpp"

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
    std::expected<code_runner::run_result, error_code> run_source_code(
        const std::filesystem::path& source_file_path
    );

private:
    explicit judge_worker(
        submission_service submission_service,
        std::string cpp_compiler_path,
        std::string python_path,
        std::string java_runtime_path
    );

    static bool is_queue_empty_error(const error_code& code);
    std::expected<void, error_code> set_testcase_paths(std::int64_t problem_id);
    std::expected<code_runner::run_result, error_code> run_cpp(const std::filesystem::path& source_file_path);
    std::expected<code_runner::run_result, error_code> run_python(const std::filesystem::path& source_file_path);
    std::expected<code_runner::run_result, error_code> run_java(const std::filesystem::path& source_file_path);
    static constexpr std::chrono::milliseconds notification_wait_timeout_{30000};
    static constexpr std::chrono::milliseconds source_run_time_limit_{2000};
    static constexpr std::int64_t source_run_memory_limit_mb_{256};

    submission_service submission_service_;
    std::filesystem::path input_path_;
    std::filesystem::path output_path_;
    std::string cpp_compiler_path_;
    std::string python_path_;
    std::string java_runtime_path_;
};
