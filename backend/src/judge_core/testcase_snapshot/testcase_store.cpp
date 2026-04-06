#include "judge_core/testcase_snapshot/testcase_store.hpp"

#include "common/file_util.hpp"
#include "judge_core/testcase_snapshot/testcase_util.hpp"

#include <string>
#include <system_error>
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

    std::expected<problem_content_dto::limits, judge_error> read_problem_limits(
        const std::filesystem::path& testcase_directory_path
    ){
        const auto memory_limit_file_path_exp =
            testcase_util::make_testcase_memory_limit_file_path(
                testcase_directory_path
            );
        if(!memory_limit_file_path_exp){
            return std::unexpected(memory_limit_file_path_exp.error());
        }

        const auto time_limit_file_path_exp =
            testcase_util::make_testcase_time_limit_file_path(
                testcase_directory_path
            );
        if(!time_limit_file_path_exp){
            return std::unexpected(time_limit_file_path_exp.error());
        }

        const auto memory_limit_exp = file_util::read_int32_file(
            *memory_limit_file_path_exp
        );
        if(!memory_limit_exp){
            return std::unexpected(memory_limit_exp.error());
        }

        const auto time_limit_exp = file_util::read_int32_file(
            *time_limit_file_path_exp
        );
        if(!time_limit_exp){
            return std::unexpected(time_limit_exp.error());
        }

        problem_content_dto::limits problem_limits_value;
        problem_limits_value.memory_mb = *memory_limit_exp;
        problem_limits_value.time_ms = *time_limit_exp;
        return problem_limits_value;
    }

    std::expected<testcase_snapshot, judge_error> make_testcase_snapshot(
        std::int64_t problem_id,
        std::int32_t version,
        const std::filesystem::path& testcase_directory_path
    ){
        const auto testcase_count_exp = testcase_util::count_testcase_output(
            testcase_directory_path
        );
        if(!testcase_count_exp){
            return std::unexpected(testcase_count_exp.error());
        }

        const auto validated_testcase_count_exp = testcase_util::validate_testcase_output(
            testcase_directory_path,
            *testcase_count_exp
        );
        if(!validated_testcase_count_exp){
            return std::unexpected(validated_testcase_count_exp.error());
        }

        const auto problem_limits_exp = read_problem_limits(testcase_directory_path);
        if(!problem_limits_exp){
            return std::unexpected(problem_limits_exp.error());
        }

        return testcase_snapshot::make(
            problem_id,
            version,
            testcase_directory_path,
            *validated_testcase_count_exp,
            *problem_limits_exp
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

std::expected<bool, judge_error> testcase_store::has_version(
    std::int64_t problem_id,
    std::int32_t version
) const{
    const auto version_directory_path_exp =
        testcase_util::make_testcase_version_directory_path(
            testcase_root_path_,
            problem_id,
            version
        );
    if(!version_directory_path_exp){
        return std::unexpected(version_directory_path_exp.error());
    }

    const auto version_directory_exists_exp = file_util::exists(
        *version_directory_path_exp
    );
    if(!version_directory_exists_exp){
        return std::unexpected(version_directory_exists_exp.error());
    }

    return *version_directory_exists_exp;
}

std::expected<testcase_store::staging_area, judge_error>
testcase_store::create_staging_area(
    std::int64_t problem_id,
    std::int32_t version
) const{
    const auto problem_directory_path_exp =
        testcase_util::make_testcase_problem_directory_path(
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
    const auto input_path_exp = testcase_util::make_testcase_input_path(
        staging_area_value.path(),
        order
    );
    if(!input_path_exp){
        return std::unexpected(input_path_exp.error());
    }

    const auto output_path_exp = testcase_util::make_testcase_output_path(
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

std::expected<void, judge_error> testcase_store::write_problem_limits(
    const staging_area& staging_area_value,
    const problem_content_dto::limits& problem_limits_value
) const{
    const auto memory_limit_file_path_exp =
        testcase_util::make_testcase_memory_limit_file_path(
            staging_area_value.path()
        );
    if(!memory_limit_file_path_exp){
        return std::unexpected(memory_limit_file_path_exp.error());
    }

    const auto time_limit_file_path_exp =
        testcase_util::make_testcase_time_limit_file_path(
            staging_area_value.path()
        );
    if(!time_limit_file_path_exp){
        return std::unexpected(time_limit_file_path_exp.error());
    }

    const auto create_directories_exp = file_util::create_directories(
        memory_limit_file_path_exp->parent_path()
    );
    if(!create_directories_exp){
        return std::unexpected(create_directories_exp.error());
    }

    const auto create_memory_limit_file_exp = file_util::create_file(
        *memory_limit_file_path_exp,
        std::to_string(problem_limits_value.memory_mb)
    );
    if(!create_memory_limit_file_exp){
        return std::unexpected(create_memory_limit_file_exp.error());
    }

    const auto create_time_limit_file_exp = file_util::create_file(
        *time_limit_file_path_exp,
        std::to_string(problem_limits_value.time_ms)
    );
    if(!create_time_limit_file_exp){
        return std::unexpected(create_time_limit_file_exp.error());
    }

    return {};
}

std::expected<void, judge_error> testcase_store::publish_version_directory(
    const staging_area& staging_area_value,
    std::int64_t problem_id,
    std::int32_t version
) const{
    const auto version_directory_path_exp =
        testcase_util::make_testcase_version_directory_path(
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
    }

    return {};
}

std::expected<testcase_snapshot, judge_error> testcase_store::load_snapshot(
    std::int64_t problem_id,
    std::int32_t version
) const{
    const auto version_directory_path_exp =
        testcase_util::make_testcase_version_directory_path(
            testcase_root_path_,
            problem_id,
            version
        );
    if(!version_directory_path_exp){
        return std::unexpected(version_directory_path_exp.error());
    }

    return make_testcase_snapshot(
        problem_id,
        version,
        *version_directory_path_exp
    );
}
