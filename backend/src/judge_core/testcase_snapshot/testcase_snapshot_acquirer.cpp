#include "judge_core/testcase_snapshot/testcase_snapshot_acquirer.hpp"

#include <limits>
#include <utility>

namespace{
    std::expected<snapshot_manifest, judge_error> make_snapshot_manifest(
        const testcase_source_facade::problem_snapshot_manifest& manifest_value
    ){
        if(
            manifest_value.testcases.size() >
            static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max())
        ){
            return std::unexpected(
                judge_error{
                    judge_error_code::validation_error,
                    "too many testcases for local snapshot manifest"
                }
            );
        }

        snapshot_manifest snapshot_manifest_value;
        snapshot_manifest_value.problem_id = manifest_value.problem_id;
        snapshot_manifest_value.version = manifest_value.version;
        snapshot_manifest_value.testcase_count =
            static_cast<std::int32_t>(manifest_value.testcases.size());
        snapshot_manifest_value.limits_value = manifest_value.limits_value;

        for(std::size_t index = 0; index < manifest_value.testcases.size(); ++index){
            if(
                manifest_value.testcases[index].order !=
                static_cast<std::int32_t>(index + 1)
            ){
                return std::unexpected(
                    judge_error{
                        judge_error_code::validation_error,
                        "problem snapshot manifest testcase order must be contiguous"
                    }
                );
            }
        }

        return snapshot_manifest_value;
    }
}

std::expected<testcase_snapshot_acquirer, judge_error>
testcase_snapshot_acquirer::create(
    testcase_source_facade testcase_source_facade_value,
    testcase_store testcase_store_value,
    std::shared_ptr<judge_runtime_registry> judge_runtime_registry
){
    return testcase_snapshot_acquirer(
        std::move(testcase_source_facade_value),
        std::move(testcase_store_value),
        std::move(judge_runtime_registry)
    );
}

testcase_snapshot_acquirer::testcase_snapshot_acquirer(
    testcase_source_facade testcase_source_facade_value,
    testcase_store testcase_store_value,
    std::shared_ptr<judge_runtime_registry> judge_runtime_registry
) :
    testcase_source_facade_(std::move(testcase_source_facade_value)),
    testcase_store_(std::move(testcase_store_value)),
    judge_runtime_registry_(std::move(judge_runtime_registry)){}

testcase_snapshot_acquirer::testcase_snapshot_acquirer(
    testcase_snapshot_acquirer&& other
) noexcept = default;

testcase_snapshot_acquirer& testcase_snapshot_acquirer::operator=(
    testcase_snapshot_acquirer&& other
) noexcept = default;

testcase_snapshot_acquirer::~testcase_snapshot_acquirer() = default;

std::expected<void, judge_error> testcase_snapshot_acquirer::download_all(
    const testcase_source_facade::problem_snapshot_manifest& manifest_value,
    const testcase_store::staging_area& staging_area_value
){
    for(const auto& testcase_value : manifest_value.testcases){
        const auto write_testcase_exp = testcase_store_.write_testcase(
            staging_area_value,
            testcase_value.order,
            testcase_value.input,
            testcase_value.output
        );
        if(!write_testcase_exp){
            return std::unexpected(write_testcase_exp.error());
        }
    }

    return {};
}

std::expected<void, judge_error>
testcase_snapshot_acquirer::sync_version_directory(
    const testcase_source_facade::problem_snapshot_manifest& manifest_value
){
    auto staging_area_exp = testcase_store_.create_staging_area(
        manifest_value.problem_id,
        manifest_value.version
    );
    if(!staging_area_exp){
        return std::unexpected(staging_area_exp.error());
    }

    auto staging_area_value = std::move(*staging_area_exp);

    const auto download_all_exp = download_all(
        manifest_value,
        staging_area_value
    );
    if(!download_all_exp){
        return std::unexpected(download_all_exp.error());
    }

    const auto snapshot_manifest_exp = make_snapshot_manifest(manifest_value);
    if(!snapshot_manifest_exp){
        return std::unexpected(snapshot_manifest_exp.error());
    }

    const auto write_manifest_exp = testcase_store_.write_manifest(
        staging_area_value,
        *snapshot_manifest_exp
    );
    if(!write_manifest_exp){
        return std::unexpected(write_manifest_exp.error());
    }

    const auto validate_staging_area_exp = testcase_store_.validate_staging_area(
        staging_area_value,
        *snapshot_manifest_exp
    );
    if(!validate_staging_area_exp){
        return std::unexpected(validate_staging_area_exp.error());
    }

    return testcase_store_.publish_version_directory(
        staging_area_value,
        manifest_value.problem_id,
        manifest_value.version
    );
}

std::expected<testcase_snapshot, judge_error>
testcase_snapshot_acquirer::acquire_testcase_snapshot(
    std::int64_t problem_id,
    std::int32_t problem_version
){
    if(problem_version <= 0){
        return std::unexpected(
            judge_error{
                judge_error_code::validation_error,
                "invalid problem version"
            }
        );
    }

    const auto snapshot_exists_exp = testcase_store_.has_valid_manifest(
        problem_id,
        problem_version
    );
    if(!snapshot_exists_exp){
        return std::unexpected(snapshot_exists_exp.error());
    }

    if(snapshot_exists_exp.value()){
        if(judge_runtime_registry_){
            judge_runtime_registry_->record_snapshot_cache_hit();
        }
    }
    else{
        if(judge_runtime_registry_){
            judge_runtime_registry_->record_snapshot_cache_miss();
        }

        const auto manifest_exp = testcase_source_facade_.fetch_manifest(
            problem_id,
            problem_version
        );
        if(!manifest_exp){
            return std::unexpected(manifest_exp.error());
        }

        const auto sync_version_directory_exp = sync_version_directory(
            *manifest_exp
        );
        if(!sync_version_directory_exp){
            return std::unexpected(sync_version_directory_exp.error());
        }
    }

    return testcase_store_.load_snapshot(
        problem_id,
        problem_version
    );
}
