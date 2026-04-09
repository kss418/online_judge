#include "judge_core/testcase_snapshot/testcase_store.hpp"

#include "common/file_util.hpp"
#include "judge_core/testcase_snapshot/snapshot_layout.hpp"
#include "judge_core/testcase_snapshot/snapshot_manifest_serializer.hpp"

#include <string>
#include <utility>

namespace{
    constexpr int FILE_OPERATION_ATTEMPT_COUNT = 5;

    std::expected<void, io_error> rename_directory(
        const std::filesystem::path& source_path,
        const std::filesystem::path& destination_path
    ){
        return file_util::retry_file_operation(
            FILE_OPERATION_ATTEMPT_COUNT,
            [&]() -> std::expected<void, io_error> {
                std::error_code rename_ec;
                std::filesystem::rename(source_path, destination_path, rename_ec);
                if(rename_ec){
                    return std::unexpected(io_error::from_error_code(rename_ec));
                }

                return std::expected<void, io_error>{};
            }
        );
    }
}

std::expected<testcase_store, judge_error> testcase_store::create(
    std::filesystem::path testcase_root_path
){
    if(testcase_root_path.empty()){
        return std::unexpected(
            judge_error{
                judge_error_code::validation_error,
                "testcase root path is not configured"
            }
        );
    }

    return testcase_store(std::move(testcase_root_path));
}

testcase_store::testcase_store(
    std::filesystem::path testcase_root_path
) :
    testcase_root_path_(std::move(testcase_root_path)){}

testcase_store::testcase_store(
    testcase_store&& other
) noexcept = default;

testcase_store& testcase_store::operator=(
    testcase_store&& other
) noexcept = default;

testcase_store::~testcase_store() = default;

std::expected<testcase_store::staging_area, judge_error>
testcase_store::create_staging_area(
    std::int64_t problem_id,
    std::int32_t version
) const{
    const auto problem_directory_path_exp = snapshot_layout::make_problem_directory_path(
        testcase_root_path_,
        problem_id
    );
    if(!problem_directory_path_exp){
        return std::unexpected(problem_directory_path_exp.error());
    }

    const auto create_problem_directory_exp = file_util::create_directories(
        *problem_directory_path_exp
    );
    if(!create_problem_directory_exp){
        return std::unexpected(create_problem_directory_exp.error());
    }

    std::string temp_directory_pattern =
        (*problem_directory_path_exp /
         ("version-" + std::to_string(version) + ".tmp.XXXXXX")).string();
    auto temp_directory_exp = temp_dir::create(temp_directory_pattern);
    if(!temp_directory_exp){
        return std::unexpected(temp_directory_exp.error());
    }

    return staging_area{std::move(*temp_directory_exp)};
}

std::expected<void, judge_error> testcase_store::write_testcase(
    const staging_area& staging_area_value,
    std::int32_t order,
    std::string_view input,
    std::string_view output
) const{
    const auto input_path_exp = snapshot_layout::make_input_path(
        staging_area_value.path(),
        order
    );
    if(!input_path_exp){
        return std::unexpected(input_path_exp.error());
    }

    const auto output_path_exp = snapshot_layout::make_output_path(
        staging_area_value.path(),
        order
    );
    if(!output_path_exp){
        return std::unexpected(output_path_exp.error());
    }

    const auto create_directories_exp = file_util::create_directories(
        input_path_exp->parent_path()
    );
    if(!create_directories_exp){
        return std::unexpected(create_directories_exp.error());
    }

    const auto create_input_exp = file_util::create_file(
        *input_path_exp,
        input
    );
    if(!create_input_exp){
        return std::unexpected(create_input_exp.error());
    }

    const auto create_output_exp = file_util::create_file(
        *output_path_exp,
        output
    );
    if(!create_output_exp){
        return std::unexpected(create_output_exp.error());
    }

    return {};
}

std::expected<void, judge_error> testcase_store::write_manifest(
    const staging_area& staging_area_value,
    const snapshot_manifest& manifest_value
) const{
    const auto manifest_path_exp = snapshot_layout::make_manifest_path(
        staging_area_value.path()
    );
    if(!manifest_path_exp){
        return std::unexpected(manifest_path_exp.error());
    }

    const auto create_directories_exp = file_util::create_directories(
        manifest_path_exp->parent_path()
    );
    if(!create_directories_exp){
        return std::unexpected(create_directories_exp.error());
    }

    const auto manifest_content_exp = snapshot_manifest_serializer::serialize(
        manifest_value
    );
    if(!manifest_content_exp){
        return std::unexpected(manifest_content_exp.error());
    }

    const auto create_manifest_file_exp = file_util::create_file(
        *manifest_path_exp,
        *manifest_content_exp
    );
    if(!create_manifest_file_exp){
        return std::unexpected(create_manifest_file_exp.error());
    }

    return {};
}

std::expected<void, judge_error> testcase_store::validate_staging_area(
    const staging_area& staging_area_value,
    const snapshot_manifest& manifest_value
) const{
    return snapshot_layout::validate_materialized_snapshot(
        staging_area_value.path(),
        manifest_value
    );
}

std::expected<void, judge_error> testcase_store::publish_version_directory(
    const staging_area& staging_area_value,
    std::int64_t problem_id,
    std::int32_t version
) const{
    const auto version_directory_path_exp = snapshot_layout::make_version_directory_path(
        testcase_root_path_,
        problem_id,
        version
    );
    if(!version_directory_path_exp){
        return std::unexpected(version_directory_path_exp.error());
    }

    const auto rename_directory_exp = rename_directory(
        staging_area_value.path(),
        *version_directory_path_exp
    );
    if(!rename_directory_exp){
        const auto exists_after_rename_exp = file_util::exists(
            *version_directory_path_exp
        );
        if(!exists_after_rename_exp){
            return std::unexpected(exists_after_rename_exp.error());
        }

        if(!exists_after_rename_exp.value()){
            return std::unexpected(rename_directory_exp.error());
        }

        const auto valid_manifest_exp = has_valid_manifest(problem_id, version);
        if(!valid_manifest_exp){
            return std::unexpected(valid_manifest_exp.error());
        }
        if(!valid_manifest_exp.value()){
            const auto remove_invalid_version_exp = file_util::remove_all(
                *version_directory_path_exp
            );
            if(!remove_invalid_version_exp){
                return std::unexpected(remove_invalid_version_exp.error());
            }

            const auto retry_rename_exp = rename_directory(
                staging_area_value.path(),
                *version_directory_path_exp
            );
            if(!retry_rename_exp){
                return std::unexpected(retry_rename_exp.error());
            }
        }
    }

    return {};
}

std::expected<snapshot_manifest, judge_error> testcase_store::read_manifest(
    std::int64_t problem_id,
    std::int32_t version
) const{
    const auto version_directory_path_exp = snapshot_layout::make_version_directory_path(
        testcase_root_path_,
        problem_id,
        version
    );
    if(!version_directory_path_exp){
        return std::unexpected(version_directory_path_exp.error());
    }

    const auto manifest_path_exp = snapshot_layout::make_manifest_path(
        *version_directory_path_exp
    );
    if(!manifest_path_exp){
        return std::unexpected(manifest_path_exp.error());
    }

    const auto manifest_content_exp = file_util::read_file_content(*manifest_path_exp);
    if(!manifest_content_exp){
        return std::unexpected(manifest_content_exp.error());
    }

    const auto manifest_exp = snapshot_manifest_serializer::parse(
        *manifest_content_exp
    );
    if(!manifest_exp){
        return std::unexpected(manifest_exp.error());
    }

    if(
        manifest_exp->problem_id != problem_id ||
        manifest_exp->version != version
    ){
        return std::unexpected(
            judge_error{
                judge_error_code::validation_error,
                "testcase snapshot manifest does not match requested version"
            }
        );
    }

    return *manifest_exp;
}

std::expected<bool, judge_error> testcase_store::has_valid_manifest(
    std::int64_t problem_id,
    std::int32_t version
) const{
    const auto manifest_exp = read_manifest(problem_id, version);
    if(manifest_exp){
        return true;
    }

    if(
        manifest_exp.error().code == judge_error_code::not_found ||
        manifest_exp.error().code == judge_error_code::validation_error
    ){
        return false;
    }

    return std::unexpected(manifest_exp.error());
}

std::expected<testcase_snapshot, judge_error> testcase_store::load_snapshot(
    std::int64_t problem_id,
    std::int32_t version
) const{
    const auto manifest_exp = read_manifest(problem_id, version);
    if(!manifest_exp){
        return std::unexpected(manifest_exp.error());
    }

    const auto version_directory_path_exp = snapshot_layout::make_version_directory_path(
        testcase_root_path_,
        problem_id,
        version
    );
    if(!version_directory_path_exp){
        return std::unexpected(version_directory_path_exp.error());
    }

    auto testcase_snapshot_value = testcase_snapshot::make(
        problem_id,
        version,
        *version_directory_path_exp,
        manifest_exp->testcase_count,
        manifest_exp->limits_value
    );

    const auto validate_exp = testcase_snapshot_value.validate();
    if(!validate_exp){
        return std::unexpected(validate_exp.error());
    }

    return testcase_snapshot_value;
}
