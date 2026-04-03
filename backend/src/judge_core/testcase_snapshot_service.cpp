#include "judge_core/testcase_snapshot_service.hpp"

#include "common/env_util.hpp"
#include "judge_core/testcase_downloader.hpp"

#include <utility>

namespace{
    std::expected<std::filesystem::path, judge_error> load_testcase_root_path(){
        const auto testcase_root_path_text_exp = env_util::require_env("TESTCASE_PATH");
        if(!testcase_root_path_text_exp){
            return std::unexpected(testcase_root_path_text_exp.error());
        }

        const std::filesystem::path testcase_root_path(*testcase_root_path_text_exp);
        if(testcase_root_path.empty()){
            return std::unexpected(
                judge_error{
                    judge_error_code::validation_error,
                    "testcase root path is not configured"
                }
            );
        }

        return testcase_root_path;
    }
}

std::expected<testcase_snapshot_service, judge_error> testcase_snapshot_service::create(
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

    const auto testcase_root_path_exp = load_testcase_root_path();
    if(!testcase_root_path_exp){
        return std::unexpected(testcase_root_path_exp.error());
    }

    return testcase_snapshot_service(
        *testcase_root_path_exp,
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
