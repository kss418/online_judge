#pragma once

#include "common/error_code.hpp"

#include <cstdint>
#include <expected>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string_view>
#include <vector>

class file_util{
public:
    static file_util& instance();

    std::expected<bool, error_code> exists(const std::filesystem::path& file_path);
    std::expected<void, error_code> create_directories(const std::filesystem::path& directory_path);
    std::expected<void, error_code> remove_file(const std::filesystem::path& file_path);
    std::expected<std::int32_t, error_code> read_int32_file(const std::filesystem::path& file_path);

    std::expected<void, error_code> create_file(
        const std::filesystem::path& file_path,
        std::string_view file_content
    );

    std::expected<std::filesystem::path, error_code> make_source_directory_path();
    std::expected<std::filesystem::path, error_code> make_source_file_path(
        std::int64_t submission_id,
        std::string_view language
    );
    std::expected<std::filesystem::path, error_code> make_tc_problem_directory_path(
        std::int64_t problem_id
    );
    std::expected<std::filesystem::path, error_code> make_tc_input_path(
        std::int64_t problem_id,
        std::int32_t order
    );
    std::expected<std::filesystem::path, error_code> make_tc_output_path(
        std::int64_t problem_id,
        std::int32_t order
    );
    std::expected<std::filesystem::path, error_code> make_tc_version_file_path(
        std::int64_t problem_id
    );
    std::expected<std::filesystem::path, error_code> make_tc_memory_limit_file_path(
        std::int64_t problem_id
    );
    std::expected<std::filesystem::path, error_code> make_tc_time_limit_file_path(
        std::int64_t problem_id
    );
    std::expected<std::int32_t, error_code> count_tc_output(std::int64_t problem_id);
    std::expected<std::int32_t, error_code> validate_tc_output(
        std::int64_t problem_id,
        std::int32_t tc_count
    );

private:
    file_util() = default;
    void initialize_if_needed();

    std::mutex initialize_mutex_;
    std::optional<std::filesystem::path> source_directory_path_;
    std::optional<std::filesystem::path> tc_root_path_;
};
