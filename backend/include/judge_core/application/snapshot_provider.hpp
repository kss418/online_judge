#pragma once

#include "error/judge_error.hpp"
#include "judge_core/gateway/testcase_snapshot_facade.hpp"
#include "judge_core/testcase_snapshot/testcase_snapshot.hpp"

#include <cstdint>
#include <expected>

class snapshot_provider{
public:
    static std::expected<snapshot_provider, judge_error> create(
        testcase_snapshot_facade testcase_snapshot_facade_value
    );

    snapshot_provider(snapshot_provider&& other) noexcept;
    snapshot_provider& operator=(snapshot_provider&& other) noexcept;
    ~snapshot_provider();

    snapshot_provider(const snapshot_provider&) = delete;
    snapshot_provider& operator=(const snapshot_provider&) = delete;

    std::expected<testcase_snapshot, judge_error> acquire(std::int64_t problem_id);

private:
    explicit snapshot_provider(
        testcase_snapshot_facade testcase_snapshot_facade_value
    );

    testcase_snapshot_facade testcase_snapshot_facade_;
};
