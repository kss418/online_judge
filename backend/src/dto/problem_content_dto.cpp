#include "dto/problem_content_dto.hpp"

#include "common/json_field_util.hpp"
#include "error/request_error.hpp"

bool problem_content_dto::is_valid(const limits& limits_value){
    return limits_value.memory_mb > 0 && limits_value.time_ms > 0;
}

bool problem_content_dto::is_valid(const statement& statement_value){
    return
        !statement_value.description.empty() &&
        !statement_value.input_format.empty() &&
        !statement_value.output_format.empty();
}

bool problem_content_dto::is_valid(const sample_ref& sample_reference_value){
    return sample_reference_value.problem_id > 0 && sample_reference_value.sample_order > 0;
}

std::expected<problem_content_dto::limits, http_error>
problem_content_dto::make_limits_from_json(const boost::json::object& json){
    const auto memory_limit_mb_opt = json_field_util::get_positive_int32_field(
        json,
        "memory_limit_mb"
    );
    if(!memory_limit_mb_opt){
        return std::unexpected(request_error::make_invalid_field_error(
            "memory_limit_mb",
            "memory_limit_mb must be a positive integer"
        ));
    }

    const auto time_limit_ms_opt = json_field_util::get_positive_int32_field(
        json,
        "time_limit_ms"
    );
    if(!time_limit_ms_opt){
        return std::unexpected(request_error::make_invalid_field_error(
            "time_limit_ms",
            "time_limit_ms must be a positive integer"
        ));
    }

    limits limits_value;
    limits_value.memory_mb = *memory_limit_mb_opt;
    limits_value.time_ms = *time_limit_ms_opt;
    return limits_value;
}

std::expected<problem_content_dto::statement, http_error>
problem_content_dto::make_statement_from_json(const boost::json::object& json){
    const auto description_opt = json_field_util::get_non_empty_string_field(
        json,
        "description"
    );
    if(!description_opt){
        return std::unexpected(request_error::make_missing_field_error("description"));
    }

    const auto input_format_opt = json_field_util::get_non_empty_string_field(
        json,
        "input_format"
    );
    if(!input_format_opt){
        return std::unexpected(request_error::make_missing_field_error("input_format"));
    }

    const auto output_format_opt = json_field_util::get_non_empty_string_field(
        json,
        "output_format"
    );
    if(!output_format_opt){
        return std::unexpected(request_error::make_missing_field_error("output_format"));
    }

    statement statement_value;
    statement_value.description = std::string{*description_opt};
    statement_value.input_format = std::string{*input_format_opt};
    statement_value.output_format = std::string{*output_format_opt};

    const auto* note_value = json.if_contains("note");
    if(note_value != nullptr){
        if(note_value->is_null()){
            statement_value.note = std::nullopt;
        }
        else if(note_value->is_string()){
            const auto& note_string = note_value->as_string();
            statement_value.note = std::string{note_string.data(), note_string.size()};
        }
        else{
            return std::unexpected(request_error::make_invalid_field_error(
                "note",
                "note must be a string or null"
            ));
        }
    }

    return statement_value;
}

std::expected<problem_content_dto::sample, http_error>
problem_content_dto::make_sample_from_json(const boost::json::object& json){
    const auto input_opt = json_field_util::get_string_field(json, "sample_input");
    if(!input_opt){
        return std::unexpected(request_error::make_missing_field_error("sample_input"));
    }

    const auto output_opt = json_field_util::get_string_field(json, "sample_output");
    if(!output_opt){
        return std::unexpected(request_error::make_missing_field_error("sample_output"));
    }

    sample sample_value;
    sample_value.input = std::string{*input_opt};
    sample_value.output = std::string{*output_opt};
    return sample_value;
}
