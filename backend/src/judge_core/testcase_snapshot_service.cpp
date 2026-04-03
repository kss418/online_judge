#include "judge_core/testcase_snapshot_service.hpp"

#include "judge_core/testcase_downloader.hpp"

#include <utility>

std::expected<testcase_snapshot_service, judge_error> testcase_snapshot_service::create(
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

    return testcase_snapshot_service(
        std::move(testcase_root_path),
        std::move(problem_lock_registry)
    );
}

testcase_snapshot_service::testcase_snapshot_service(
    std::filesystem::path testcase_root_path,
    std::shared_ptr<problem_lock_registry> problem_lock_registry
) :
    testcase_root_path_(std::move(testcase_root_path)),
    problem_lock_registry_(std::move(problem_lock_registry)){}

std::expected<testcase_snapshot, judge_error> testcase_snapshot_service::acquire(
    db_connection& connection,
    std::int64_t problem_id
){
    auto problem_lock_exp = problem_lock_registry_->lock(problem_id);
    if(!problem_lock_exp){
        return std::unexpected(problem_lock_exp.error());
    }

    [[maybe_unused]] auto problem_lock = std::move(*problem_lock_exp);
    return testcase_downloader::ensure_testcase_snapshot(
        connection,
        testcase_root_path_,
        problem_id
    );
}
