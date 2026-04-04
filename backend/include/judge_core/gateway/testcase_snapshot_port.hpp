#pragma once

#include "common/db_connection.hpp"
#include "error/judge_error.hpp"
#include "judge_core/infrastructure/problem_lock_registry.hpp"
#include "judge_core/types/testcase_snapshot.hpp"

#include <cstdint>
#include <expected>
#include <filesystem>
#include <memory>

class testcase_snapshot_port{
public:
    static std::expected<testcase_snapshot_port, judge_error> create(
        const db_connection_config& db_config,
        std::filesystem::path testcase_root_path,
        std::shared_ptr<problem_lock_registry> problem_lock_registry
    );

    testcase_snapshot_port(testcase_snapshot_port&& other) noexcept;
    testcase_snapshot_port& operator=(testcase_snapshot_port&& other) noexcept;
    ~testcase_snapshot_port();

    testcase_snapshot_port(const testcase_snapshot_port&) = delete;
    testcase_snapshot_port& operator=(const testcase_snapshot_port&) = delete;

    std::expected<testcase_snapshot, judge_error> acquire(std::int64_t problem_id);

private:
    testcase_snapshot_port(
        db_connection db_connection_value,
        std::filesystem::path testcase_root_path,
        std::shared_ptr<problem_lock_registry> problem_lock_registry
    );

    db_connection db_connection_;
    std::filesystem::path testcase_root_path_;
    std::shared_ptr<problem_lock_registry> problem_lock_registry_;
};
