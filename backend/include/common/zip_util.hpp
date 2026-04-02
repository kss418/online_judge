#pragma once

#include "error/error_code.hpp"

#include <expected>
#include <filesystem>
#include <string>
#include <vector>

namespace zip_util{
    std::expected<std::vector<std::string>, error_code> list_entry_names(
        const std::filesystem::path& archive_path
    );

    std::expected<void, error_code> extract_to_directory(
        const std::filesystem::path& archive_path,
        const std::filesystem::path& output_directory_path
    );
}
