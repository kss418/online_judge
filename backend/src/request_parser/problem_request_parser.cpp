#include "request_parser/problem_request_parser.hpp"

#include "common/json_field_util.hpp"
#include "common/query_param_util.hpp"
#include "common/string_util.hpp"
#include "error/request_error.hpp"

#include <array>
#include <optional>
#include <string>

namespace{
    std::optional<std::int64_t> parse_non_negative_int64(std::string_view raw_value){
        if(raw_value == "0"){
            return std::int64_t{0};
        }

        return string_util::parse_positive_int64(raw_value);
    }

    std::optional<std::string> parse_problem_list_state(std::string_view raw_value){
        if(raw_value == "solved" || raw_value == "unsolved"){
            return std::string{raw_value};
        }

        return std::nullopt;
    }

    std::optional<std::string> parse_problem_list_sort(std::string_view raw_value){
        if(
            raw_value == "problem_id" ||
            raw_value == "accepted_count" ||
            raw_value == "acceptance_rate" ||
            raw_value == "submission_count"
        ){
            return std::string{raw_value};
        }

        return std::nullopt;
    }

    std::optional<std::string> parse_sort_direction(std::string_view raw_value){
        if(raw_value == "asc" || raw_value == "desc"){
            return std::string{raw_value};
        }

        return std::nullopt;
    }

    const std::array<
        query_param_util::query_param_binding<problem_dto::list_filter>,
        7
    > problem_list_filter_bindings{{
        {
            "problem_id",
            [](problem_dto::list_filter& filter_value,
               std::string_view key,
               std::string_view raw_value) -> std::expected<void, http_error> {
                return query_param_util::parse_unique_query_param(
                    filter_value.problem_id_opt,
                    key,
                    raw_value,
                    string_util::parse_positive_int64,
                    request_error::query_param_error_adapter::duplicate,
                    request_error::query_param_error_adapter::invalid
                );
            }
        },
        {
            "title",
            [](problem_dto::list_filter& filter_value,
               std::string_view key,
               std::string_view raw_value) -> std::expected<void, http_error> {
                return query_param_util::parse_unique_query_param(
                    filter_value.title_opt,
                    key,
                    raw_value,
                    [](std::string_view value) -> std::optional<std::string> {
                        if(value.empty()){
                            return std::nullopt;
                        }

                        return std::string{value};
                    },
                    request_error::query_param_error_adapter::duplicate,
                    request_error::query_param_error_adapter::invalid
                );
            }
        },
        {
            "state",
            [](problem_dto::list_filter& filter_value,
               std::string_view key,
               std::string_view raw_value) -> std::expected<void, http_error> {
                return query_param_util::parse_unique_query_param(
                    filter_value.state_opt,
                    key,
                    raw_value,
                    parse_problem_list_state,
                    request_error::query_param_error_adapter::duplicate,
                    request_error::query_param_error_adapter::invalid
                );
            }
        },
        {
            "sort",
            [](problem_dto::list_filter& filter_value,
               std::string_view key,
               std::string_view raw_value) -> std::expected<void, http_error> {
                return query_param_util::parse_unique_query_param(
                    filter_value.sort_opt,
                    key,
                    raw_value,
                    parse_problem_list_sort,
                    request_error::query_param_error_adapter::duplicate,
                    request_error::query_param_error_adapter::invalid
                );
            }
        },
        {
            "direction",
            [](problem_dto::list_filter& filter_value,
               std::string_view key,
               std::string_view raw_value) -> std::expected<void, http_error> {
                return query_param_util::parse_unique_query_param(
                    filter_value.direction_opt,
                    key,
                    raw_value,
                    parse_sort_direction,
                    request_error::query_param_error_adapter::duplicate,
                    request_error::query_param_error_adapter::invalid
                );
            }
        },
        {
            "limit",
            [](problem_dto::list_filter& filter_value,
               std::string_view key,
               std::string_view raw_value) -> std::expected<void, http_error> {
                return query_param_util::parse_unique_query_param(
                    filter_value.limit_opt,
                    key,
                    raw_value,
                    string_util::parse_positive_int32,
                    request_error::query_param_error_adapter::duplicate,
                    request_error::query_param_error_adapter::invalid
                );
            }
        },
        {
            "offset",
            [](problem_dto::list_filter& filter_value,
               std::string_view key,
               std::string_view raw_value) -> std::expected<void, http_error> {
                return query_param_util::parse_unique_query_param(
                    filter_value.offset_opt,
                    key,
                    raw_value,
                    parse_non_negative_int64,
                    request_error::query_param_error_adapter::duplicate,
                    request_error::query_param_error_adapter::invalid
                );
            }
        }
    }};
}

std::expected<problem_dto::create_request, http_error>
problem_request_parser::parse_create_request(const boost::json::object& json){
    const auto title_opt = json_field_util::get_non_empty_string_field(json, "title");
    if(!title_opt){
        return std::unexpected(request_error::make_missing_field_error("title"));
    }

    problem_dto::create_request create_request_value;
    create_request_value.title = std::string{*title_opt};
    return create_request_value;
}

std::expected<problem_dto::update_request, http_error>
problem_request_parser::parse_update_request(const boost::json::object& json){
    const auto title_opt = json_field_util::get_non_empty_string_field(json, "title");
    if(!title_opt){
        return std::unexpected(request_error::make_missing_field_error("title"));
    }

    problem_dto::update_request update_request_value;
    update_request_value.title = std::string{*title_opt};
    return update_request_value;
}

std::expected<problem_dto::list_filter, http_error>
problem_request_parser::parse_list_filter(const std::vector<query_param>& query_params){
    return query_param_util::make_filter_from_query_params(
        query_params,
        problem_list_filter_bindings,
        request_error::query_param_error_adapter::unsupported
    );
}

std::expected<void, http_error> problem_request_parser::validate_list_filter_for_viewer(
    const problem_dto::list_filter& filter_value,
    bool has_viewer_user
){
    if(filter_value.state_opt && !has_viewer_user){
        return std::unexpected(request_error::make_invalid_query_parameter_error("state"));
    }

    return {};
}

std::expected<problem_dto::testcase, http_error>
problem_request_parser::parse_testcase(const boost::json::object& json){
    const auto input_opt = json_field_util::get_string_field(json, "testcase_input");
    if(!input_opt){
        return std::unexpected(request_error::make_missing_field_error("testcase_input"));
    }

    const auto output_opt = json_field_util::get_string_field(json, "testcase_output");
    if(!output_opt){
        return std::unexpected(request_error::make_missing_field_error("testcase_output"));
    }

    problem_dto::testcase testcase_value;
    testcase_value.input = std::string{*input_opt};
    testcase_value.output = std::string{*output_opt};
    return testcase_value;
}

std::expected<problem_dto::testcase_move_request, http_error>
problem_request_parser::parse_testcase_move_request(const boost::json::object& json){
    const auto source_testcase_order_opt = json_field_util::get_positive_int32_field(
        json,
        "source_testcase_order"
    );
    if(!source_testcase_order_opt){
        return std::unexpected(request_error::make_invalid_field_error(
            "source_testcase_order",
            "source_testcase_order must be a positive integer"
        ));
    }

    const auto target_testcase_order_opt = json_field_util::get_positive_int32_field(
        json,
        "target_testcase_order"
    );
    if(!target_testcase_order_opt){
        return std::unexpected(request_error::make_invalid_field_error(
            "target_testcase_order",
            "target_testcase_order must be a positive integer"
        ));
    }

    problem_dto::testcase_move_request testcase_move_request_value;
    testcase_move_request_value.source_testcase_order = *source_testcase_order_opt;
    testcase_move_request_value.target_testcase_order = *target_testcase_order_opt;
    return testcase_move_request_value;
}
