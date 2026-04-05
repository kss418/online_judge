#pragma once

#include "error/judge_error.hpp"
#include "judge_core/gateway/testcase_source_port.hpp"
#include "judge_core/infrastructure/testcase_store.hpp"
#include "judge_core/types/testcase_snapshot.hpp"

#include <cstdint>
#include <expected>

class testcase_downloader{
public:
    static std::expected<testcase_downloader, judge_error> create(
        testcase_source_port testcase_source_port_value,
        testcase_store testcase_store_value
    );

    testcase_downloader(testcase_downloader&& other) noexcept;
    testcase_downloader& operator=(testcase_downloader&& other) noexcept;
    ~testcase_downloader();

    testcase_downloader(const testcase_downloader&) = delete;
    testcase_downloader& operator=(const testcase_downloader&) = delete;

    std::expected<testcase_snapshot, judge_error> ensure_testcase_snapshot(
        std::int64_t problem_id
    );

private:
    testcase_downloader(
        testcase_source_port testcase_source_port_value,
        testcase_store testcase_store_value
    );

    std::expected<void, judge_error> download_one(
        std::int64_t problem_id,
        std::int32_t order,
        const testcase_store::staging_area& staging_area_value
    );

    std::expected<void, judge_error> download_all(
        std::int64_t problem_id,
        const testcase_store::staging_area& staging_area_value
    );

    std::expected<void, judge_error> sync_version_directory(
        std::int64_t problem_id,
        std::int32_t version,
        const problem_content_dto::limits& problem_limits_value
    );

    testcase_source_port testcase_source_port_;
    testcase_store testcase_store_;
};
