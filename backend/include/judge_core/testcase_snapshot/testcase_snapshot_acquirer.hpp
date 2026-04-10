#pragma once

#include "error/judge_error.hpp"
#include "judge_core/gateway/testcase_source_facade.hpp"
#include "judge_core/infrastructure/judge_runtime_registry.hpp"
#include "judge_core/testcase_snapshot/testcase_snapshot.hpp"
#include "judge_core/testcase_snapshot/testcase_store.hpp"

#include <cstdint>
#include <expected>
#include <memory>

class testcase_snapshot_acquirer{
public:
    static std::expected<testcase_snapshot_acquirer, judge_error> create(
        testcase_source_facade testcase_source_facade_value,
        testcase_store testcase_store_value,
        std::shared_ptr<judge_runtime_registry> judge_runtime_registry
    );

    testcase_snapshot_acquirer(testcase_snapshot_acquirer&& other) noexcept;
    testcase_snapshot_acquirer& operator=(testcase_snapshot_acquirer&& other) noexcept;
    ~testcase_snapshot_acquirer();

    testcase_snapshot_acquirer(const testcase_snapshot_acquirer&) = delete;
    testcase_snapshot_acquirer& operator=(const testcase_snapshot_acquirer&) = delete;

    std::expected<testcase_snapshot, judge_error> acquire_testcase_snapshot(
        std::int64_t problem_id,
        std::int32_t problem_version
    );

private:
    testcase_snapshot_acquirer(
        testcase_source_facade testcase_source_facade_value,
        testcase_store testcase_store_value,
        std::shared_ptr<judge_runtime_registry> judge_runtime_registry
    );

    std::expected<void, judge_error> download_all(
        const testcase_source_facade::problem_snapshot_manifest& manifest_value,
        const testcase_store::staging_area& staging_area_value
    );

    std::expected<void, judge_error> sync_version_directory(
        const testcase_source_facade::problem_snapshot_manifest& manifest_value
    );

    testcase_source_facade testcase_source_facade_;
    testcase_store testcase_store_;
    std::shared_ptr<judge_runtime_registry> judge_runtime_registry_;
};
