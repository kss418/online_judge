#pragma once

#include "error/io_error.hpp"

#include <cstdint>
#include <expected>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

enum class judge_result{
    accepted,
    wrong_answer,
    time_limit_exceeded,
    memory_limit_exceeded,
    runtime_error,
    compile_error,
    output_exceeded,
    invalid_output
};

namespace judge_workspace{
    std::expected<std::filesystem::path, io_error> make_submission_workspace_path(
        const std::filesystem::path& source_root_path,
        std::int64_t submission_id
    );
    std::expected<void, io_error> reset(const std::filesystem::path& workspace_path);
    std::expected<std::filesystem::path, io_error> make_source_file_path(
        const std::filesystem::path& workspace_path,
        std::string_view language
    );
    std::expected<std::filesystem::path, io_error> write_source_file(
        const std::filesystem::path& workspace_path,
        std::string_view language,
        std::string_view source_code
    );
    std::expected<void, io_error> cleanup(const std::filesystem::path& workspace_path);
    std::filesystem::path sandbox_workspace_path();
    std::filesystem::path make_sandbox_path(
        const std::filesystem::path& host_workspace_path,
        const std::filesystem::path& host_path
    );
}
