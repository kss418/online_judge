#include "dto/problem_dto.hpp"

#include "http_server/http_util.hpp"

std::expected<problem_dto::create_request, dto_validation_error>
problem_dto::make_create_request_from_json(const boost::json::object& json){
    const auto title_opt = http_util::get_non_empty_string_field(json, "title");
    if(!title_opt){
        return std::unexpected(dto_validation_error{
            .code = "missing_field",
            .message = "required field: title",
            .field_opt = "title"
        });
    }

    create_request create_request_value;
    create_request_value.title = std::string{*title_opt};
    return create_request_value;
}

std::expected<problem_dto::update_request, dto_validation_error>
problem_dto::make_update_request_from_json(const boost::json::object& json){
    const auto title_opt = http_util::get_non_empty_string_field(json, "title");
    if(!title_opt){
        return std::unexpected(dto_validation_error{
            .code = "missing_field",
            .message = "required field: title",
            .field_opt = "title"
        });
    }

    update_request update_request_value;
    update_request_value.title = std::string{*title_opt};
    return update_request_value;
}

std::expected<problem_dto::list_filter, dto_validation_error>
problem_dto::make_list_filter_from_query_params(
    const std::vector<http_util::query_param>& query_params
){
    list_filter filter_value;
    for(const auto& query_param : query_params){
        if(query_param.key == "title"){
            if(filter_value.title_opt){
                return std::unexpected(dto_validation_error{
                    .code = "duplicate_query_parameter",
                    .message = "duplicate query parameter: title",
                    .field_opt = "title"
                });
            }

            if(query_param.value.empty()){
                return std::unexpected(dto_validation_error{
                    .code = "invalid_query_parameter",
                    .message = "invalid query parameter: title",
                    .field_opt = "title"
                });
            }

            filter_value.title_opt = std::string{query_param.value};
            continue;
        }

        return std::unexpected(dto_validation_error{
            .code = "unsupported_query_parameter",
            .message = "unsupported query parameter: " + std::string{query_param.key},
            .field_opt = std::string{query_param.key}
        });
    }

    return filter_value;
}

std::expected<problem_dto::testcase, dto_validation_error> problem_dto::make_testcase_from_json(
    const boost::json::object& json
){
    const auto input_opt = http_util::get_string_field(json, "testcase_input");
    if(!input_opt){
        return std::unexpected(dto_validation_error{
            .code = "missing_field",
            .message = "required field: testcase_input",
            .field_opt = "testcase_input"
        });
    }

    const auto output_opt = http_util::get_string_field(json, "testcase_output");
    if(!output_opt){
        return std::unexpected(dto_validation_error{
            .code = "missing_field",
            .message = "required field: testcase_output",
            .field_opt = "testcase_output"
        });
    }

    testcase testcase_value;
    testcase_value.input = std::string{*input_opt};
    testcase_value.output = std::string{*output_opt};
    return testcase_value;
}
