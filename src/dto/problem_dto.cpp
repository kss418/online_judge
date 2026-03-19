#include "dto/problem_dto.hpp"

#include "http_server/http_util.hpp"

std::optional<problem_dto::limits> problem_dto::make_limits(
    const boost::json::object& request_object
){
    const auto memory_limit_mb_opt = http_util::get_positive_int32_field(
        request_object,
        "memory_limit_mb"
    );
    const auto time_limit_ms_opt = http_util::get_positive_int32_field(
        request_object,
        "time_limit_ms"
    );
    if(!memory_limit_mb_opt || !time_limit_ms_opt){
        return std::nullopt;
    }

    limits limits_value;
    limits_value.memory_mb = *memory_limit_mb_opt;
    limits_value.time_ms = *time_limit_ms_opt;
    return limits_value;
}

std::optional<problem_dto::statement> problem_dto::make_statement(
    const boost::json::object& request_object
){
    const auto description_opt = http_util::get_non_empty_string_field(
        request_object,
        "description"
    );
    const auto input_format_opt = http_util::get_non_empty_string_field(
        request_object,
        "input_format"
    );
    const auto output_format_opt = http_util::get_non_empty_string_field(
        request_object,
        "output_format"
    );
    if(!description_opt || !input_format_opt || !output_format_opt){
        return std::nullopt;
    }

    statement statement_value;
    statement_value.description = std::string{*description_opt};
    statement_value.input_format = std::string{*input_format_opt};
    statement_value.output_format = std::string{*output_format_opt};

    const auto* note_value = request_object.if_contains("note");
    if(note_value != nullptr){
        if(note_value->is_null()){
            statement_value.note = std::nullopt;
        }
        else if(note_value->is_string()){
            const auto& note_string = note_value->as_string();
            statement_value.note = std::string{note_string.data(), note_string.size()};
        }
        else{
            return std::nullopt;
        }
    }

    return statement_value;
}

std::optional<problem_dto::testcase> problem_dto::make_testcase(
    const boost::json::object& request_object
){
    const auto input_opt = http_util::get_string_field(request_object, "testcase_input");
    const auto output_opt = http_util::get_string_field(request_object, "testcase_output");
    if(!input_opt || !output_opt){
        return std::nullopt;
    }

    testcase testcase_value;
    testcase_value.input = std::string{*input_opt};
    testcase_value.output = std::string{*output_opt};
    return testcase_value;
}
