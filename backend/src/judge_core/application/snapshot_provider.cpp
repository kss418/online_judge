#include "judge_core/application/snapshot_provider.hpp"

#include <utility>

std::expected<snapshot_provider, judge_error> snapshot_provider::create(
    testcase_snapshot_facade testcase_snapshot_facade_value
){
    return snapshot_provider(std::move(testcase_snapshot_facade_value));
}

snapshot_provider::snapshot_provider(
    testcase_snapshot_facade testcase_snapshot_facade_value
) :
    testcase_snapshot_facade_(std::move(testcase_snapshot_facade_value)){}

snapshot_provider::snapshot_provider(
    snapshot_provider&& other
) noexcept = default;

snapshot_provider& snapshot_provider::operator=(
    snapshot_provider&& other
) noexcept = default;

snapshot_provider::~snapshot_provider() = default;

std::expected<testcase_snapshot, judge_error> snapshot_provider::acquire(
    std::int64_t problem_id
){
    return testcase_snapshot_facade_.acquire(problem_id);
}
