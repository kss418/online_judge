#pragma once

#include "common/db_connection.hpp"
#include "error/judge_error.hpp"
#include "judge_core/infrastructure/problem_lock_registry.hpp"
#include "judge_core/testcase_snapshot/testcase_snapshot.hpp"
#include "judge_core/testcase_snapshot/testcase_snapshot_acquirer.hpp"

#include <cstdint>
#include <expected>
#include <filesystem>
#include <memory>


class testcase_snapshot_facade{
public:
    static std::expected<testcase_snapshot_facade, judge_error> create(
        const db_connection_config& db_config,
        std::filesystem::path testcase_root_path,
        std::shared_ptr<problem_lock_registry> problem_lock_registry
    );

    testcase_snapshot_facade(testcase_snapshot_facade&& other) noexcept;
    testcase_snapshot_facade& operator=(testcase_snapshot_facade&& other) noexcept;
    ~testcase_snapshot_facade();

    testcase_snapshot_facade(const testcase_snapshot_facade&) = delete;
    testcase_snapshot_facade& operator=(const testcase_snapshot_facade&) = delete;

    std::expected<testcase_snapshot, judge_error> acquire(std::int64_t problem_id);

private:
    testcase_snapshot_facade(
        testcase_snapshot_acquirer testcase_snapshot_acquirer_value,
        std::shared_ptr<problem_lock_registry> problem_lock_registry
    );

    testcase_snapshot_acquirer testcase_snapshot_acquirer_;
    std::shared_ptr<problem_lock_registry> problem_lock_registry_;
};
