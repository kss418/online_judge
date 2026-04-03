#pragma once

#include "error/zip_error.hpp"

#include <expected>
#include <filesystem>
#include <string>
#include <vector>

namespace zip_util{
    std::expected<std::vector<std::string>, zip_error> list_entry_names(
        const std::filesystem::path& archive_path
    );

    std::expected<void, zip_error> extract_to_directory(
        const std::filesystem::path& archive_path,
        const std::filesystem::path& output_directory_path
    );
}
