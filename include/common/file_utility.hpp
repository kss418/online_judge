#pragma once

#include "common/error_code.hpp"

#include <cstdint>
#include <expected>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string_view>

class file_utility{
public:
    static file_utility& instance();

    std::expected<bool, error_code> exists(const std::filesystem::path& file_path);
    std::expected<void, error_code> create_directories(const std::filesystem::path& directory_path);
    std::expected<void, error_code> remove_file(const std::filesystem::path& file_path);

    std::expected<void, error_code> create_file(
        const std::filesystem::path& file_path,
        std::string_view file_content
    );

    std::expected<std::filesystem::path, error_code> make_source_directory_path();
    std::expected<std::filesystem::path, error_code> make_source_file_path(
        std::int64_t submission_id,
        std::string_view language
    );
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

private:
    file_utility() = default;
    void initialize_if_needed();

    std::mutex initialize_mutex_;
    std::optional<std::filesystem::path> source_directory_path_;
    std::optional<std::filesystem::path> testcase_root_path_;
};
