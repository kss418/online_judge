#include "dto/problem_content_dto.hpp"

#include "http_core/http_util.hpp"

std::expected<problem_content_dto::limits, dto_validation_error>
problem_content_dto::make_limits_from_json(const boost::json::object& json){
    const auto memory_limit_mb_opt = http_util::get_positive_int32_field(
        json,
        "memory_limit_mb"
    );
    if(!memory_limit_mb_opt){
        return std::unexpected(dto_validation_error{
            .code = "invalid_field",
            .message = "memory_limit_mb must be a positive integer",
            .field_opt = "memory_limit_mb"
        });
    }

    const auto time_limit_ms_opt = http_util::get_positive_int32_field(
        json,
        "time_limit_ms"
    );
    if(!time_limit_ms_opt){
        return std::unexpected(dto_validation_error{
            .code = "invalid_field",
            .message = "time_limit_ms must be a positive integer",
            .field_opt = "time_limit_ms"
        });
    }

    limits limits_value;
    limits_value.memory_mb = *memory_limit_mb_opt;
    limits_value.time_ms = *time_limit_ms_opt;
    return limits_value;
}

std::expected<problem_content_dto::statement, dto_validation_error>
problem_content_dto::make_statement_from_json(const boost::json::object& json){
    const auto description_opt = http_util::get_non_empty_string_field(
        json,
        "description"
    );
    if(!description_opt){
        return std::unexpected(dto_validation_error{
            .code = "missing_field",
            .message = "required field: description",
            .field_opt = "description"
        });
    }

    const auto input_format_opt = http_util::get_non_empty_string_field(
        json,
        "input_format"
    );
    if(!input_format_opt){
        return std::unexpected(dto_validation_error{
            .code = "missing_field",
            .message = "required field: input_format",
            .field_opt = "input_format"
        });
    }

    const auto output_format_opt = http_util::get_non_empty_string_field(
        json,
        "output_format"
    );
    if(!output_format_opt){
        return std::unexpected(dto_validation_error{
            .code = "missing_field",
            .message = "required field: output_format",
            .field_opt = "output_format"
        });
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
            return std::unexpected(dto_validation_error{
                .code = "invalid_field",
                .message = "note must be a string or null",
                .field_opt = "note"
            });
        }
    }

    return statement_value;
}

std::expected<problem_content_dto::sample, dto_validation_error>
problem_content_dto::make_sample_from_json(const boost::json::object& json){
    const auto input_opt = http_util::get_string_field(json, "sample_input");
    if(!input_opt){
        return std::unexpected(dto_validation_error{
            .code = "missing_field",
            .message = "required field: sample_input",
            .field_opt = "sample_input"
        });
    }

    const auto output_opt = http_util::get_string_field(json, "sample_output");
    if(!output_opt){
        return std::unexpected(dto_validation_error{
            .code = "missing_field",
            .message = "required field: sample_output",
            .field_opt = "sample_output"
        });
    }

    sample sample_value;
    sample_value.input = std::string{*input_opt};
    sample_value.output = std::string{*output_opt};
    return sample_value;
}
