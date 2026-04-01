#pragma once

#include "dto/problem_dto.hpp"

#include <expected>
#include <string>
#include <string_view>
#include <vector>

namespace testcase_zip_service{
    enum class error_kind{
        create_temp_file_failed,
        write_temp_file_failed,
        finalize_temp_file_failed,
        inspect_zip_unavailable,
        inspect_zip_invalid,
        inspect_zip_failed,
        invalid_archive_entries,
        create_extract_directory_failed,
        extract_zip_unavailable,
        extract_zip_invalid,
        extract_zip_failed,
        load_testcases_failed
    };

    struct error{
        error_kind kind;
        std::string detail;
    };

    std::expected<std::vector<problem_dto::testcase>, error> load_testcases_from_zip_body(
        std::string_view zip_body
    );
}
