#pragma once

#include "common/error_code.hpp"

#include <cstdint>
#include <expected>
#include <filesystem>
#include <string_view>

namespace file_util{
    std::expected<bool, error_code> exists(const std::filesystem::path& file_path);
    std::expected<void, error_code> create_directories(const std::filesystem::path& directory_path);
    std::expected<void, error_code> remove_file(const std::filesystem::path& file_path);
    std::expected<std::int32_t, error_code> read_int32_file(const std::filesystem::path& file_path);

    std::expected<void, error_code> create_file(
        const std::filesystem::path& file_path,
        std::string_view file_content
    );
}
