#pragma once

#include "common/db_connection.hpp"
#include "error/judge_error.hpp"
#include "judge_core/testcase_snapshot.hpp"

#include <cstdint>
#include <expected>
#include <filesystem>
#include <utility>

class testcase_downloader{
public:
    static std::expected<testcase_downloader, judge_error> create(db_connection connection);
    std::expected<testcase_snapshot, judge_error> ensure_testcase_snapshot(
        std::int64_t problem_id
    );

private:
    std::expected<std::int32_t, judge_error> fetch_problem_version(std::int64_t problem_id);
    std::expected<problem_content_dto::limits, judge_error> fetch_problem_limits(
        std::int64_t problem_id
    );
    std::expected<problem_content_dto::limits, judge_error> read_problem_limits(
        const std::filesystem::path& testcase_directory_path
    ) const;
    std::expected<testcase_snapshot, judge_error> make_testcase_snapshot(
        std::int64_t problem_id,
        std::int32_t version,
        const std::filesystem::path& testcase_directory_path
    ) const;
    std::expected<void, judge_error> sync_version_directory(
        const std::filesystem::path& testcase_root_path,
        std::int64_t problem_id,
        std::int32_t version,
        const problem_content_dto::limits& problem_limits_value
    );
    std::expected<void, judge_error> sync_limit_file(
        const problem_content_dto::limits& problem_limits_value,
        const std::filesystem::path& testcase_directory_path
    );
    std::expected<void, judge_error> download_all(
        std::int64_t problem_id,
        const std::filesystem::path& testcase_directory_path
    );
    std::expected<void, judge_error> download_one(
        std::int64_t problem_id, 
        std::int32_t order,
        const std::filesystem::path& testcase_directory_path
    );
    
    explicit testcase_downloader(db_connection connection);

    db_connection connection_;
};
