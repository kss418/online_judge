#pragma once

#include "error/io_error.hpp"
#include "error/judge_error.hpp"
#include "judge_core/testcase_snapshot/snapshot_manifest.hpp"

#include <cstdint>
#include <expected>
#include <filesystem>

namespace snapshot_layout{
    std::expected<std::filesystem::path, io_error> make_problem_directory_path(
        const std::filesystem::path& testcase_root_path,
        std::int64_t problem_id
    );

    std::expected<std::filesystem::path, io_error> make_version_directory_path(
        const std::filesystem::path& testcase_root_path,
        std::int64_t problem_id,
        std::int32_t version
    );

    std::expected<std::filesystem::path, io_error> make_input_path(
        const std::filesystem::path& testcase_directory_path,
        std::int32_t order
    );

    std::expected<std::filesystem::path, io_error> make_output_path(
        const std::filesystem::path& testcase_directory_path,
        std::int32_t order
    );

    std::expected<std::filesystem::path, io_error> make_manifest_path(
        const std::filesystem::path& testcase_directory_path
    );

    std::expected<void, judge_error> validate_materialized_snapshot(
        const std::filesystem::path& testcase_directory_path,
        const snapshot_manifest& manifest_value
    );
}
