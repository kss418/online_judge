#include "judge_core/gateway/testcase_snapshot_facade.hpp"

#include "judge_core/infrastructure/testcase_snapshot_materializer.hpp"

#include <utility>

std::expected<testcase_snapshot_facade, judge_error> testcase_snapshot_facade::create(
    const db_connection_config& db_config,
    std::filesystem::path testcase_root_path,
    std::shared_ptr<problem_lock_registry> problem_lock_registry
){
    if(!problem_lock_registry){
        return std::unexpected(
            judge_error{
                judge_error_code::internal,
                "problem lock registry is not configured"
            }
        );
    }

    if(testcase_root_path.empty()){
        return std::unexpected(
            judge_error{
                judge_error_code::validation_error,
                "testcase root path is not configured"
            }
        );
    }

    auto testcase_source_facade_exp = testcase_source_facade::create(db_config);
    if(!testcase_source_facade_exp){
        return std::unexpected(testcase_source_facade_exp.error());
    }

    auto testcase_store_exp = testcase_store::create(
        std::move(testcase_root_path)
    );
    if(!testcase_store_exp){
        return std::unexpected(testcase_store_exp.error());
    }

    auto testcase_snapshot_materializer_exp =
        testcase_snapshot_materializer::create(
        std::move(*testcase_source_facade_exp),
        std::move(*testcase_store_exp)
    );
    if(!testcase_snapshot_materializer_exp){
        return std::unexpected(testcase_snapshot_materializer_exp.error());
    }

    return testcase_snapshot_facade(
        std::move(*testcase_snapshot_materializer_exp),
        std::move(problem_lock_registry)
    );
}

testcase_snapshot_facade::testcase_snapshot_facade(
    testcase_snapshot_materializer testcase_snapshot_materializer_value,
    std::shared_ptr<problem_lock_registry> problem_lock_registry
) :
    testcase_snapshot_materializer_(std::move(testcase_snapshot_materializer_value)),
    problem_lock_registry_(std::move(problem_lock_registry)){}

testcase_snapshot_facade::testcase_snapshot_facade(
    testcase_snapshot_facade&& other
) noexcept = default;

testcase_snapshot_facade& testcase_snapshot_facade::operator=(
    testcase_snapshot_facade&& other
) noexcept = default;

testcase_snapshot_facade::~testcase_snapshot_facade() = default;

std::expected<testcase_snapshot, judge_error> testcase_snapshot_facade::acquire(
    std::int64_t problem_id
){
    auto problem_lock_exp = problem_lock_registry_->lock(problem_id);
    if(!problem_lock_exp){
        return std::unexpected(problem_lock_exp.error());
    }

    [[maybe_unused]] auto problem_lock = std::move(*problem_lock_exp);
    return testcase_snapshot_materializer_.ensure_testcase_snapshot(
        problem_id
    );
}
