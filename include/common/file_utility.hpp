#pragma once

#include "common/error_code.hpp"

#include <cstdint>
#include <expected>
#include <filesystem>
#include <string_view>

namespace file_utility{
    std::expected<void, error_code> create_file(
        const std::filesystem::path& file_path,
        std::string_view file_content
    );

    std::filesystem::path make_source_file_path(
        const std::filesystem::path& source_root_path,
        std::int64_t submission_id,
        std::string_view language
    );
}
