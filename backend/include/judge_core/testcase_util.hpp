#pragma once

#include "common/error_code.hpp"

#include <cstdint>
#include <expected>
#include <filesystem>
#include <mutex>
#include <optional>

class testcase_util{
public:
    static testcase_util& instance();

    std::expected<std::filesystem::path, error_code> make_testcase_problem_directory_path(
        std::int64_t problem_id
    );
    std::expected<std::filesystem::path, error_code> make_testcase_input_path(
        std::int64_t problem_id,
        std::int32_t order
    );
    std::expected<std::filesystem::path, error_code> make_testcase_output_path(
        std::int64_t problem_id,
        std::int32_t order
    );
    std::expected<std::filesystem::path, error_code> make_testcase_version_file_path(
        std::int64_t problem_id
    );
    std::expected<std::filesystem::path, error_code> make_testcase_memory_limit_file_path(
        std::int64_t problem_id
    );
    std::expected<std::filesystem::path, error_code> make_testcase_time_limit_file_path(
        std::int64_t problem_id
    );
    std::expected<std::int32_t, error_code> count_testcase_output(std::int64_t problem_id);
    std::expected<std::int32_t, error_code> validate_testcase_output(
        std::int64_t problem_id,
        std::int32_t testcase_count
    );

private:
    testcase_util() = default;
    void initialize_if_needed();

    std::mutex initialize_mutex_;
    std::optional<std::filesystem::path> testcase_root_path_;
};
