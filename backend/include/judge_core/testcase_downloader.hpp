#pragma once

#include "common/error_code.hpp"
#include "common/db_connection.hpp"

#include <cstdint>
#include <expected>
#include <filesystem>
#include <utility>

class testcase_downloader{
public:
    static std::expected<testcase_downloader, error_code> create(db_connection connection);
    std::expected<std::filesystem::path, error_code> sync_testcases(std::int64_t problem_id);

private:
    std::expected<std::int32_t, error_code> fetch_problem_version(std::int64_t problem_id);
    std::expected<void, error_code> sync_version_directory(
        std::int64_t problem_id,
        std::int32_t version
    );
    std::expected<void, error_code> sync_limit_file(
        std::int64_t problem_id,
        const std::filesystem::path& testcase_directory_path
    );
    std::expected<void, error_code> download_all(
        std::int64_t problem_id,
        const std::filesystem::path& testcase_directory_path
    );
    std::expected<void, error_code> download_one(
        std::int64_t problem_id, 
        std::int32_t order,
        const std::filesystem::path& testcase_directory_path
    );
    
    explicit testcase_downloader(db_connection connection);

    db_connection connection_;
};
