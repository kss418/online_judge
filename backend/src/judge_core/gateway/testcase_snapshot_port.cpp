#include "judge_core/gateway/testcase_snapshot_port.hpp"

#include "judge_core/infrastructure/testcase_downloader.hpp"

#include <utility>

std::expected<testcase_snapshot_port, judge_error> testcase_snapshot_port::create(
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

    auto db_connection_exp = db_connection::create(db_config);
    if(!db_connection_exp){
        return std::unexpected(judge_error{db_connection_exp.error()});
    }

    return testcase_snapshot_port(
        std::move(*db_connection_exp),
        std::move(testcase_root_path),
        std::move(problem_lock_registry)
    );
}

testcase_snapshot_port::testcase_snapshot_port(
    db_connection db_connection_value,
    std::filesystem::path testcase_root_path,
    std::shared_ptr<problem_lock_registry> problem_lock_registry
) :
    db_connection_(std::move(db_connection_value)),
    testcase_root_path_(std::move(testcase_root_path)),
    problem_lock_registry_(std::move(problem_lock_registry)){}

testcase_snapshot_port::testcase_snapshot_port(
    testcase_snapshot_port&& other
) noexcept = default;

testcase_snapshot_port& testcase_snapshot_port::operator=(
    testcase_snapshot_port&& other
) noexcept = default;

testcase_snapshot_port::~testcase_snapshot_port() = default;

std::expected<testcase_snapshot, judge_error> testcase_snapshot_port::acquire(
    std::int64_t problem_id
){
    auto problem_lock_exp = problem_lock_registry_->lock(problem_id);
    if(!problem_lock_exp){
        return std::unexpected(problem_lock_exp.error());
    }

    [[maybe_unused]] auto problem_lock = std::move(*problem_lock_exp);
    return testcase_downloader::ensure_testcase_snapshot(
        db_connection_,
        testcase_root_path_,
        problem_id
    );
}
