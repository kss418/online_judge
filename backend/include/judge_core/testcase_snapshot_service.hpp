#pragma once

#include "common/db_connection.hpp"
#include "error/judge_error.hpp"
#include "judge_core/problem_lock_registry.hpp"
#include "judge_core/testcase_snapshot.hpp"

#include <cstdint>
#include <expected>
#include <filesystem>
#include <memory>

class testcase_snapshot_service{
public:
    static std::expected<testcase_snapshot_service, judge_error> create(
        std::shared_ptr<problem_lock_registry> problem_lock_registry
    );

    std::expected<testcase_snapshot, judge_error> acquire(
        db_connection& connection,
        std::int64_t problem_id
    );

private:
    testcase_snapshot_service(
        std::filesystem::path testcase_root_path,
        std::shared_ptr<problem_lock_registry> problem_lock_registry
    );

    std::filesystem::path testcase_root_path_;
    std::shared_ptr<problem_lock_registry> problem_lock_registry_;
};
