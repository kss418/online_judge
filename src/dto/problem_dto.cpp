#include "dto/problem_dto.hpp"

#include "http_server/http_util.hpp"

std::optional<problem_dto::tc> problem_dto::make_tc(
    const boost::json::object& request_object
){
    const auto input_opt = http_util::get_string_field(request_object, "testcase_input");
    const auto output_opt = http_util::get_string_field(request_object, "testcase_output");
    if(!input_opt || !output_opt){
        return std::nullopt;
    }

    tc testcase_value;
    testcase_value.input = std::string{*input_opt};
    testcase_value.output = std::string{*output_opt};
    return testcase_value;
}
