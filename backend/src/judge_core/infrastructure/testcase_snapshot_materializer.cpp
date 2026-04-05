#include "judge_core/infrastructure/testcase_snapshot_materializer.hpp"

#include <utility>

std::expected<testcase_snapshot_materializer, judge_error>
testcase_snapshot_materializer::create(
    testcase_source_port testcase_source_port_value,
    testcase_store testcase_store_value
){
    return testcase_snapshot_materializer(
        std::move(testcase_source_port_value),
        std::move(testcase_store_value)
    );
}

testcase_snapshot_materializer::testcase_snapshot_materializer(
    testcase_source_port testcase_source_port_value,
    testcase_store testcase_store_value
) :
    testcase_source_port_(std::move(testcase_source_port_value)),
    testcase_store_(std::move(testcase_store_value)){}

testcase_snapshot_materializer::testcase_snapshot_materializer(
    testcase_snapshot_materializer&& other
) noexcept = default;

testcase_snapshot_materializer& testcase_snapshot_materializer::operator=(
    testcase_snapshot_materializer&& other
) noexcept = default;

testcase_snapshot_materializer::~testcase_snapshot_materializer() = default;

std::expected<void, judge_error> testcase_snapshot_materializer::download_one(
    std::int64_t problem_id,
    std::int32_t order,
    const testcase_store::staging_area& staging_area_value
){
    const auto testcase_exp = testcase_source_port_.fetch_testcase(
        problem_id,
        order
    );
    if(!testcase_exp){
        return std::unexpected(testcase_exp.error());
    }

    return testcase_store_.write_testcase(
        staging_area_value,
        order,
        testcase_exp->input,
        testcase_exp->output
    );
}

std::expected<void, judge_error> testcase_snapshot_materializer::download_all(
    std::int64_t problem_id,
    const testcase_store::staging_area& staging_area_value
){
    const auto testcase_count_exp = testcase_source_port_.fetch_testcase_count(
        problem_id
    );
    if(!testcase_count_exp){
        return std::unexpected(testcase_count_exp.error());
    }

    for(std::int32_t order = 1; order <= *testcase_count_exp; ++order){
        const auto download_one_exp = download_one(
            problem_id,
            order,
            staging_area_value
        );
        if(!download_one_exp){
            return std::unexpected(download_one_exp.error());
        }
    }

    return {};
}

std::expected<void, judge_error>
testcase_snapshot_materializer::sync_version_directory(
    std::int64_t problem_id,
    std::int32_t version,
    const problem_content_dto::limits& problem_limits_value
){
    auto staging_area_exp = testcase_store_.create_staging_area(
        problem_id,
        version
    );
    if(!staging_area_exp){
        return std::unexpected(staging_area_exp.error());
    }

    auto staging_area_value = std::move(*staging_area_exp);

    const auto download_all_exp = download_all(
        problem_id,
        staging_area_value
    );
    if(!download_all_exp){
        return std::unexpected(download_all_exp.error());
    }

    const auto write_problem_limits_exp = testcase_store_.write_problem_limits(
        staging_area_value,
        problem_limits_value
    );
    if(!write_problem_limits_exp){
        return std::unexpected(write_problem_limits_exp.error());
    }

    return testcase_store_.publish_version_directory(
        staging_area_value,
        problem_id,
        version
    );
}

std::expected<testcase_snapshot, judge_error>
testcase_snapshot_materializer::ensure_testcase_snapshot(std::int64_t problem_id){
    const auto version_exp = testcase_source_port_.fetch_problem_version(
        problem_id
    );
    if(!version_exp){
        return std::unexpected(version_exp.error());
    }

    const auto has_version_exp = testcase_store_.has_version(
        problem_id,
        *version_exp
    );
    if(!has_version_exp){
        return std::unexpected(has_version_exp.error());
    }

    if(!has_version_exp.value()){
        const auto problem_limits_exp = testcase_source_port_.fetch_problem_limits(
            problem_id
        );
        if(!problem_limits_exp){
            return std::unexpected(problem_limits_exp.error());
        }

        const auto sync_version_directory_exp = sync_version_directory(
            problem_id,
            *version_exp,
            *problem_limits_exp
        );
        if(!sync_version_directory_exp){
            return std::unexpected(sync_version_directory_exp.error());
        }
    }

    return testcase_store_.load_snapshot(
        problem_id,
        *version_exp
    );
}
