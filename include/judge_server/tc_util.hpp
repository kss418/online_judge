#pragma once

#include "common/error_code.hpp"

#include <cstdint>
#include <expected>
#include <filesystem>
#include <mutex>
#include <optional>

class tc_util{
public:
    static tc_util& instance();

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
    tc_util() = default;
    void initialize_if_needed();

    std::mutex initialize_mutex_;
    std::optional<std::filesystem::path> tc_root_path_;
};
