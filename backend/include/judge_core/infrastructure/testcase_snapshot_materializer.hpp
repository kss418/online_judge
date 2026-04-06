#pragma once

#include "error/judge_error.hpp"
#include "judge_core/gateway/testcase_source_facade.hpp"
#include "judge_core/infrastructure/testcase_store.hpp"
#include "judge_core/types/testcase_snapshot.hpp"

#include <cstdint>
#include <expected>

class testcase_snapshot_materializer{
public:
    static std::expected<testcase_snapshot_materializer, judge_error> create(
        testcase_source_facade testcase_source_facade_value,
        testcase_store testcase_store_value
    );

    testcase_snapshot_materializer(testcase_snapshot_materializer&& other) noexcept;
    testcase_snapshot_materializer& operator=(testcase_snapshot_materializer&& other) noexcept;
    ~testcase_snapshot_materializer();

    testcase_snapshot_materializer(const testcase_snapshot_materializer&) = delete;
    testcase_snapshot_materializer& operator=(const testcase_snapshot_materializer&) = delete;

    std::expected<testcase_snapshot, judge_error> ensure_testcase_snapshot(
        std::int64_t problem_id
    );

private:
    testcase_snapshot_materializer(
        testcase_source_facade testcase_source_facade_value,
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

    testcase_source_facade testcase_source_facade_;
    testcase_store testcase_store_;
};
