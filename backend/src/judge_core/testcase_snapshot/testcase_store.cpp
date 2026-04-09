#include "judge_core/testcase_snapshot/testcase_store.hpp"

#include "common/file_util.hpp"

#include <boost/json.hpp>

#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace{
    constexpr int FILE_OPERATION_ATTEMPT_COUNT = 5;

    io_error make_invalid_argument_error(const char* message){
        return io_error{io_error_code::invalid_argument, message};
    }

    io_error make_invalid_manifest_error(std::string_view message){
        return io_error{
            io_error_code::invalid_argument,
            std::string(message)
        };
    }

    std::expected<std::filesystem::path, io_error> validate_testcase_base_path(
        const std::filesystem::path& testcase_base_path
    ){
        if(testcase_base_path.empty()){
            return std::unexpected(make_invalid_argument_error("invalid testcase base path"));
        }

        return testcase_base_path;
    }

    std::expected<std::filesystem::path, io_error> make_validated_testcase_path(
        const std::filesystem::path& testcase_base_path,
        std::filesystem::path relative_path
    ){
        const auto validated_testcase_base_path_exp = validate_testcase_base_path(
            testcase_base_path
        );
        if(!validated_testcase_base_path_exp){
            return std::unexpected(validated_testcase_base_path_exp.error());
        }

        return *validated_testcase_base_path_exp / std::move(relative_path);
    }

    std::string format_testcase_file_name(
        std::int32_t order,
        std::string_view extension
    ){
        std::ostringstream file_name_stream;
        file_name_stream << std::setw(3) << std::setfill('0') << order << extension;
        return file_name_stream.str();
    }

    std::expected<std::filesystem::path, io_error> make_testcase_problem_directory_path(
        const std::filesystem::path& testcase_root_path,
        std::int64_t problem_id
    ){
        if(problem_id <= 0){
            return std::unexpected(make_invalid_argument_error("invalid problem id"));
        }

        return make_validated_testcase_path(
            testcase_root_path,
            std::filesystem::path(std::to_string(problem_id))
        );
    }

    std::expected<std::filesystem::path, io_error> make_testcase_version_directory_path(
        const std::filesystem::path& testcase_root_path,
        std::int64_t problem_id,
        std::int32_t version
    ){
        if(version <= 0){
            return std::unexpected(make_invalid_argument_error("invalid testcase version"));
        }

        const auto problem_directory_path_exp = make_testcase_problem_directory_path(
            testcase_root_path,
            problem_id
        );
        if(!problem_directory_path_exp){
            return std::unexpected(problem_directory_path_exp.error());
        }

        return make_validated_testcase_path(
            *problem_directory_path_exp,
            std::filesystem::path(std::to_string(version))
        );
    }

    std::expected<std::filesystem::path, io_error> make_testcase_input_path(
        const std::filesystem::path& testcase_directory_path,
        std::int32_t order
    ){
        if(order <= 0){
            return std::unexpected(make_invalid_argument_error("invalid testcase order"));
        }

        return make_validated_testcase_path(
            testcase_directory_path,
            std::filesystem::path(format_testcase_file_name(order, ".in"))
        );
    }

    std::expected<std::filesystem::path, io_error> make_testcase_output_path(
        const std::filesystem::path& testcase_directory_path,
        std::int32_t order
    ){
        if(order <= 0){
            return std::unexpected(make_invalid_argument_error("invalid testcase order"));
        }

        return make_validated_testcase_path(
            testcase_directory_path,
            std::filesystem::path(format_testcase_file_name(order, ".out"))
        );
    }

    std::expected<std::filesystem::path, io_error> make_testcase_memory_limit_file_path(
        const std::filesystem::path& testcase_directory_path
    ){
        return make_validated_testcase_path(
            testcase_directory_path,
            std::filesystem::path("memory_limit")
        );
    }

    std::expected<std::filesystem::path, io_error> make_testcase_time_limit_file_path(
        const std::filesystem::path& testcase_directory_path
    ){
        return make_validated_testcase_path(
            testcase_directory_path,
            std::filesystem::path("time_limit")
        );
    }

    std::expected<std::filesystem::path, io_error> make_manifest_path(
        const std::filesystem::path& testcase_directory_path
    ){
        return make_validated_testcase_path(
            testcase_directory_path,
            std::filesystem::path("manifest.json")
        );
    }

    std::expected<std::int64_t, io_error> read_required_json_int64(
        const boost::json::object& object,
        std::string_view field_name
    ){
        const auto field_it = object.find(field_name);
        if(field_it == object.end()){
            return std::unexpected(
                make_invalid_manifest_error(
                    std::string("missing testcase snapshot manifest field: ") +
                    std::string(field_name)
                )
            );
        }

        if(field_it->value().is_int64()){
            return field_it->value().as_int64();
        }

        if(field_it->value().is_uint64()){
            const auto value = field_it->value().as_uint64();
            if(value <= static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())){
                return static_cast<std::int64_t>(value);
            }
        }

        return std::unexpected(
            make_invalid_manifest_error(
                std::string("invalid testcase snapshot manifest integer field: ") +
                std::string(field_name)
            )
        );
    }

    std::expected<std::int32_t, io_error> read_required_json_int32(
        const boost::json::object& object,
        std::string_view field_name
    ){
        const auto value_exp = read_required_json_int64(object, field_name);
        if(!value_exp){
            return std::unexpected(value_exp.error());
        }

        if(
            *value_exp < std::numeric_limits<std::int32_t>::min() ||
            *value_exp > std::numeric_limits<std::int32_t>::max()
        ){
            return std::unexpected(
                make_invalid_manifest_error(
                    std::string("out of range testcase snapshot manifest integer field: ") +
                    std::string(field_name)
                )
            );
        }

        return static_cast<std::int32_t>(*value_exp);
    }

    std::expected<problem_content_dto::limits, io_error> read_required_limits(
        const boost::json::object& manifest_object
    ){
        const auto limits_it = manifest_object.find("limits");
        if(limits_it == manifest_object.end() || !limits_it->value().is_object()){
            return std::unexpected(
                make_invalid_manifest_error("missing testcase snapshot manifest limits")
            );
        }

        const auto& limits_object = limits_it->value().as_object();
        const auto memory_limit_exp = read_required_json_int32(
            limits_object,
            "memory_limit_mb"
        );
        if(!memory_limit_exp){
            return std::unexpected(memory_limit_exp.error());
        }

        const auto time_limit_exp = read_required_json_int32(
            limits_object,
            "time_limit_ms"
        );
        if(!time_limit_exp){
            return std::unexpected(time_limit_exp.error());
        }

        problem_content_dto::limits limits_value;
        limits_value.memory_mb = *memory_limit_exp;
        limits_value.time_ms = *time_limit_exp;
        if(!problem_content_dto::is_valid(limits_value)){
            return std::unexpected(
                make_invalid_manifest_error("invalid testcase snapshot manifest limits")
            );
        }

        return limits_value;
    }

    boost::json::object make_manifest_object(
        const testcase_store::snapshot_manifest& manifest_value
    ){
        boost::json::object limits_object;
        limits_object["memory_limit_mb"] = manifest_value.limits_value.memory_mb;
        limits_object["time_limit_ms"] = manifest_value.limits_value.time_ms;

        boost::json::object manifest_object;
        manifest_object["schema_version"] = manifest_value.schema_version;
        manifest_object["problem_id"] = manifest_value.problem_id;
        manifest_object["version"] = manifest_value.version;
        manifest_object["testcase_count"] = manifest_value.testcase_count;
        manifest_object["limits"] = std::move(limits_object);
        return manifest_object;
    }

    std::expected<testcase_store::snapshot_manifest, judge_error> parse_manifest_content(
        std::string_view manifest_content
    ){
        boost::system::error_code parse_ec;
        auto manifest_json = boost::json::parse(manifest_content, parse_ec);
        if(parse_ec || !manifest_json.is_object()){
            return std::unexpected(
                judge_error{
                    judge_error_code::validation_error,
                    "invalid testcase snapshot manifest"
                }
            );
        }

        const auto& manifest_object = manifest_json.as_object();
        testcase_store::snapshot_manifest manifest_value;

        const auto schema_version_exp = read_required_json_int32(
            manifest_object,
            "schema_version"
        );
        if(!schema_version_exp){
            return std::unexpected(judge_error{schema_version_exp.error()});
        }
        manifest_value.schema_version = *schema_version_exp;

        const auto problem_id_exp = read_required_json_int64(
            manifest_object,
            "problem_id"
        );
        if(!problem_id_exp){
            return std::unexpected(judge_error{problem_id_exp.error()});
        }
        manifest_value.problem_id = *problem_id_exp;

        const auto version_exp = read_required_json_int32(manifest_object, "version");
        if(!version_exp){
            return std::unexpected(judge_error{version_exp.error()});
        }
        manifest_value.version = *version_exp;

        const auto testcase_count_exp = read_required_json_int32(
            manifest_object,
            "testcase_count"
        );
        if(!testcase_count_exp){
            return std::unexpected(judge_error{testcase_count_exp.error()});
        }
        manifest_value.testcase_count = *testcase_count_exp;

        const auto limits_exp = read_required_limits(manifest_object);
        if(!limits_exp){
            return std::unexpected(judge_error{limits_exp.error()});
        }
        manifest_value.limits_value = *limits_exp;

        if(
            manifest_value.schema_version !=
                testcase_store::snapshot_manifest::current_schema_version ||
            manifest_value.problem_id <= 0 ||
            manifest_value.version <= 0 ||
            manifest_value.testcase_count < 0
        ){
            return std::unexpected(
                judge_error{
                    judge_error_code::validation_error,
                    "invalid testcase snapshot manifest"
                }
            );
        }

        return manifest_value;
    }

    std::expected<std::int32_t, io_error> count_testcase_output(
        const std::filesystem::path& testcase_directory_path
    ){
        const auto validated_directory_path_exp = validate_testcase_base_path(
            testcase_directory_path
        );
        if(!validated_directory_path_exp){
            return std::unexpected(validated_directory_path_exp.error());
        }

        std::error_code iterator_ec;
        std::filesystem::directory_iterator directory_it(
            *validated_directory_path_exp,
            iterator_ec
        );
        if(iterator_ec){
            return std::unexpected(io_error::from_error_code(iterator_ec));
        }

        std::int32_t testcase_count = 0;
        for(std::filesystem::directory_iterator end_it; directory_it != end_it; ++directory_it){
            if(directory_it->path().extension() == ".out"){
                ++testcase_count;
            }
        }

        return testcase_count;
    }

    std::expected<void, judge_error> ensure_path_exists(
        const std::filesystem::path& path,
        std::string_view message
    ){
        const auto exists_exp = file_util::exists(path);
        if(!exists_exp){
            return std::unexpected(exists_exp.error());
        }

        if(!exists_exp.value()){
            return std::unexpected(
                judge_error{
                    judge_error_code::validation_error,
                    std::string(message)
                }
            );
        }

        return {};
    }

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

    std::expected<void, judge_error> validate_snapshot_materialization(
        const std::filesystem::path& testcase_directory_path,
        const testcase_store::snapshot_manifest& manifest_value
    ){
        if(
            manifest_value.schema_version !=
                testcase_store::snapshot_manifest::current_schema_version ||
            manifest_value.problem_id <= 0 ||
            manifest_value.version <= 0 ||
            manifest_value.testcase_count < 0 ||
            !problem_content_dto::is_valid(manifest_value.limits_value)
        ){
            return std::unexpected(
                judge_error{
                    judge_error_code::validation_error,
                    "invalid testcase snapshot manifest"
                }
            );
        }

        const auto memory_limit_file_path_exp =
            make_testcase_memory_limit_file_path(testcase_directory_path);
        if(!memory_limit_file_path_exp){
            return std::unexpected(memory_limit_file_path_exp.error());
        }

        const auto time_limit_file_path_exp =
            make_testcase_time_limit_file_path(testcase_directory_path);
        if(!time_limit_file_path_exp){
            return std::unexpected(time_limit_file_path_exp.error());
        }

        const auto manifest_path_exp = make_manifest_path(testcase_directory_path);
        if(!manifest_path_exp){
            return std::unexpected(manifest_path_exp.error());
        }

        const auto memory_limit_exists_exp = ensure_path_exists(
            *memory_limit_file_path_exp,
            "missing testcase memory limit file"
        );
        if(!memory_limit_exists_exp){
            return std::unexpected(memory_limit_exists_exp.error());
        }

        const auto time_limit_exists_exp = ensure_path_exists(
            *time_limit_file_path_exp,
            "missing testcase time limit file"
        );
        if(!time_limit_exists_exp){
            return std::unexpected(time_limit_exists_exp.error());
        }

        const auto manifest_exists_exp = ensure_path_exists(
            *manifest_path_exp,
            "missing testcase snapshot manifest"
        );
        if(!manifest_exists_exp){
            return std::unexpected(manifest_exists_exp.error());
        }

        for(std::int32_t order = 1; order <= manifest_value.testcase_count; ++order){
            const auto input_path_exp = make_testcase_input_path(
                testcase_directory_path,
                order
            );
            if(!input_path_exp){
                return std::unexpected(input_path_exp.error());
            }

            const auto output_path_exp = make_testcase_output_path(
                testcase_directory_path,
                order
            );
            if(!output_path_exp){
                return std::unexpected(output_path_exp.error());
            }

            const auto input_exists_exp = ensure_path_exists(
                *input_path_exp,
                "missing testcase input file"
            );
            if(!input_exists_exp){
                return std::unexpected(input_exists_exp.error());
            }

            const auto output_exists_exp = ensure_path_exists(
                *output_path_exp,
                "missing testcase output file"
            );
            if(!output_exists_exp){
                return std::unexpected(output_exists_exp.error());
            }
        }

        const auto testcase_count_exp = count_testcase_output(testcase_directory_path);
        if(!testcase_count_exp){
            return std::unexpected(testcase_count_exp.error());
        }

        if(*testcase_count_exp != manifest_value.testcase_count){
            return std::unexpected(
                judge_error{
                    judge_error_code::validation_error,
                    "testcase output layout does not match manifest"
                }
            );
        }

        return {};
    }

    std::expected<testcase_snapshot, judge_error> make_testcase_snapshot(
        std::int64_t problem_id,
        std::int32_t version,
        const std::filesystem::path& testcase_directory_path,
        const testcase_store::snapshot_manifest& manifest_value
    ){
        auto testcase_snapshot_value = testcase_snapshot::make(
            problem_id,
            version,
            testcase_directory_path,
            manifest_value.testcase_count,
            manifest_value.limits_value
        );

        const auto validate_exp = testcase_snapshot_value.validate();
        if(!validate_exp){
            return std::unexpected(validate_exp.error());
        }

        return testcase_snapshot_value;
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
        make_testcase_version_directory_path(
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
        make_testcase_problem_directory_path(
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
    const auto input_path_exp = make_testcase_input_path(
        staging_area_value.path(),
        order
    );
    if(!input_path_exp){
        return std::unexpected(input_path_exp.error());
    }

    const auto output_path_exp = make_testcase_output_path(
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
        make_testcase_memory_limit_file_path(
            staging_area_value.path()
        );
    if(!memory_limit_file_path_exp){
        return std::unexpected(memory_limit_file_path_exp.error());
    }

    const auto time_limit_file_path_exp =
        make_testcase_time_limit_file_path(
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

std::expected<void, judge_error> testcase_store::write_manifest(
    const staging_area& staging_area_value,
    const snapshot_manifest& manifest_value
) const{
    if(
        manifest_value.schema_version != snapshot_manifest::current_schema_version ||
        manifest_value.problem_id <= 0 ||
        manifest_value.version <= 0 ||
        manifest_value.testcase_count < 0 ||
        !problem_content_dto::is_valid(manifest_value.limits_value)
    ){
        return std::unexpected(
            judge_error{
                judge_error_code::validation_error,
                "invalid testcase snapshot manifest"
            }
        );
    }

    const auto manifest_path_exp = make_manifest_path(staging_area_value.path());
    if(!manifest_path_exp){
        return std::unexpected(manifest_path_exp.error());
    }

    const auto create_directories_exp = file_util::create_directories(
        manifest_path_exp->parent_path()
    );
    if(!create_directories_exp){
        return std::unexpected(create_directories_exp.error());
    }

    std::string manifest_content = boost::json::serialize(
        make_manifest_object(manifest_value)
    );
    manifest_content.push_back('\n');

    const auto create_manifest_file_exp = file_util::create_file(
        *manifest_path_exp,
        manifest_content
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
    return validate_snapshot_materialization(
        staging_area_value.path(),
        manifest_value
    );
}

std::expected<void, judge_error> testcase_store::publish_version_directory(
    const staging_area& staging_area_value,
    std::int64_t problem_id,
    std::int32_t version
) const{
    const auto version_directory_path_exp =
        make_testcase_version_directory_path(
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
            return std::unexpected(
                judge_error{
                    judge_error_code::conflict,
                    "testcase snapshot version directory exists without a valid manifest"
                }
            );
        }
    }

    return {};
}

std::expected<testcase_store::snapshot_manifest, judge_error> testcase_store::read_manifest(
    std::int64_t problem_id,
    std::int32_t version
) const{
    const auto version_directory_path_exp =
        make_testcase_version_directory_path(
            testcase_root_path_,
            problem_id,
            version
        );
    if(!version_directory_path_exp){
        return std::unexpected(version_directory_path_exp.error());
    }

    const auto manifest_path_exp = make_manifest_path(*version_directory_path_exp);
    if(!manifest_path_exp){
        return std::unexpected(manifest_path_exp.error());
    }

    const auto manifest_content_exp = file_util::read_file_content(*manifest_path_exp);
    if(!manifest_content_exp){
        return std::unexpected(manifest_content_exp.error());
    }

    const auto manifest_exp = parse_manifest_content(*manifest_content_exp);
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

    const auto version_directory_path_exp =
        make_testcase_version_directory_path(
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
        *version_directory_path_exp,
        *manifest_exp
    );
}
