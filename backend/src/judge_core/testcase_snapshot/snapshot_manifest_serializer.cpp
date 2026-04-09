#include "judge_core/testcase_snapshot/snapshot_manifest_serializer.hpp"

#include <boost/json.hpp>

#include <limits>
#include <string>
#include <utility>

namespace{
    judge_error make_invalid_manifest_error(std::string message){
        return judge_error{
            judge_error_code::validation_error,
            std::move(message)
        };
    }

    std::expected<void, judge_error> validate_manifest(
        const snapshot_manifest& manifest_value
    ){
        if(
            manifest_value.schema_version != snapshot_manifest::current_schema_version ||
            manifest_value.problem_id <= 0 ||
            manifest_value.version <= 0 ||
            manifest_value.testcase_count < 0 ||
            !problem_content_dto::is_valid(manifest_value.limits_value)
        ){
            return std::unexpected(
                make_invalid_manifest_error("invalid testcase snapshot manifest")
            );
        }

        return {};
    }

    std::expected<std::int64_t, judge_error> read_required_json_int64(
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

    std::expected<std::int32_t, judge_error> read_required_json_int32(
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

    std::expected<problem_content_dto::limits, judge_error> read_required_limits(
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
}

std::expected<std::string, judge_error> snapshot_manifest_serializer::serialize(
    const snapshot_manifest& manifest_value
){
    const auto validate_exp = validate_manifest(manifest_value);
    if(!validate_exp){
        return std::unexpected(validate_exp.error());
    }

    boost::json::object limits_object;
    limits_object["memory_limit_mb"] = manifest_value.limits_value.memory_mb;
    limits_object["time_limit_ms"] = manifest_value.limits_value.time_ms;

    boost::json::object manifest_object;
    manifest_object["schema_version"] = manifest_value.schema_version;
    manifest_object["problem_id"] = manifest_value.problem_id;
    manifest_object["version"] = manifest_value.version;
    manifest_object["testcase_count"] = manifest_value.testcase_count;
    manifest_object["limits"] = std::move(limits_object);

    std::string manifest_content = boost::json::serialize(manifest_object);
    manifest_content.push_back('\n');
    return manifest_content;
}

std::expected<snapshot_manifest, judge_error> snapshot_manifest_serializer::parse(
    std::string_view manifest_content
){
    boost::system::error_code parse_ec;
    auto manifest_json = boost::json::parse(manifest_content, parse_ec);
    if(parse_ec || !manifest_json.is_object()){
        return std::unexpected(
            make_invalid_manifest_error("invalid testcase snapshot manifest")
        );
    }

    const auto& manifest_object = manifest_json.as_object();
    snapshot_manifest manifest_value;

    const auto schema_version_exp = read_required_json_int32(
        manifest_object,
        "schema_version"
    );
    if(!schema_version_exp){
        return std::unexpected(schema_version_exp.error());
    }
    manifest_value.schema_version = *schema_version_exp;

    const auto problem_id_exp = read_required_json_int64(
        manifest_object,
        "problem_id"
    );
    if(!problem_id_exp){
        return std::unexpected(problem_id_exp.error());
    }
    manifest_value.problem_id = *problem_id_exp;

    const auto version_exp = read_required_json_int32(manifest_object, "version");
    if(!version_exp){
        return std::unexpected(version_exp.error());
    }
    manifest_value.version = *version_exp;

    const auto testcase_count_exp = read_required_json_int32(
        manifest_object,
        "testcase_count"
    );
    if(!testcase_count_exp){
        return std::unexpected(testcase_count_exp.error());
    }
    manifest_value.testcase_count = *testcase_count_exp;

    const auto limits_exp = read_required_limits(manifest_object);
    if(!limits_exp){
        return std::unexpected(limits_exp.error());
    }
    manifest_value.limits_value = *limits_exp;

    const auto validate_exp = validate_manifest(manifest_value);
    if(!validate_exp){
        return std::unexpected(validate_exp.error());
    }

    return manifest_value;
}
