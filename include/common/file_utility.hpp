#pragma once

#include "common/error_code.hpp"

#include <cstdint>
#include <expected>
#include <filesystem>
#include <string_view>

namespace file_utility{
    std::expected<bool, error_code> exists(const std::filesystem::path& file_path);
    std::expected<void, error_code> create_directories(const std::filesystem::path& directory_path);
    std::expected<void, error_code> remove_file(const std::filesystem::path& file_path);

    std::expected<void, error_code> create_file(
        const std::filesystem::path& file_path,
        std::string_view file_content
    );

    std::filesystem::path make_source_file_path(
        const std::filesystem::path& source_root_path,
        std::int64_t submission_id,
        std::string_view language
    );
    std::filesystem::path make_testcase_problem_directory_path(
        const std::filesystem::path& testcase_root_path,
        std::int64_t problem_id
    );
    std::filesystem::path make_testcase_input_path(
        const std::filesystem::path& testcase_root_path,
        std::int64_t problem_id,
        std::int32_t order
    );
    std::filesystem::path make_testcase_output_path(
        const std::filesystem::path& testcase_root_path,
        std::int64_t problem_id,
        std::int32_t order
    );
    std::filesystem::path make_testcase_version_file_path(
        const std::filesystem::path& testcase_root_path,
        std::int64_t problem_id
    );
}
