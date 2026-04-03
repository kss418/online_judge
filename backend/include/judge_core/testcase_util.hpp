#pragma once

#include "error/io_error.hpp"

#include <cstdint>
#include <expected>
#include <filesystem>

namespace testcase_util{
    std::expected<std::filesystem::path, io_error> make_testcase_problem_directory_path(
        const std::filesystem::path& testcase_root_path,
        std::int64_t problem_id
    );
    std::expected<std::filesystem::path, io_error> make_testcase_version_directory_path(
        const std::filesystem::path& testcase_root_path,
        std::int64_t problem_id,
        std::int32_t version
    );
    std::expected<std::filesystem::path, io_error> make_testcase_input_path(
        const std::filesystem::path& testcase_directory_path,
        std::int32_t order
    );
    std::expected<std::filesystem::path, io_error> make_testcase_output_path(
        const std::filesystem::path& testcase_directory_path,
        std::int32_t order
    );
    std::expected<std::filesystem::path, io_error> make_testcase_memory_limit_file_path(
        const std::filesystem::path& testcase_directory_path
    );
    std::expected<std::filesystem::path, io_error> make_testcase_time_limit_file_path(
        const std::filesystem::path& testcase_directory_path
    );
    std::expected<std::int32_t, io_error> count_testcase_output(
        const std::filesystem::path& testcase_directory_path
    );
    std::expected<std::int32_t, io_error> validate_testcase_output(
        const std::filesystem::path& testcase_directory_path,
        std::int32_t testcase_count
    );
}
