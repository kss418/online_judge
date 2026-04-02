#pragma once

#include "error/error_code.hpp"
#include "dto/problem_dto.hpp"

#include <cstdint>
#include <expected>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace testcase_uploader{
    struct archive_entry{
        std::string input_name;
        std::string output_name;
    };

    using archive_entry_map = std::map<std::int32_t, archive_entry>;

    std::expected<archive_entry_map, std::string> parse_testcase_archive_entries(
        const std::vector<std::string>& entry_names
    );

    std::expected<std::vector<problem_dto::testcase>, error_code> load_testcases_from_directory(
        const std::filesystem::path& directory_path,
        const archive_entry_map& archive_entries
    );
}
