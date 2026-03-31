#include "dto/problem_dto.hpp"

#include "common/string_util.hpp"
#include "http_core/http_util.hpp"
#include "http_core/query_param_util.hpp"

#include <pqxx/pqxx>

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
               std::string_view raw_value) -> std::expected<void, dto_validation_error> {
                return query_param_util::parse_unique_query_param(
                    filter_value.problem_id_opt,
                    key,
                    raw_value,
                    string_util::parse_positive_int64
                );
            }
        },
        {
            "title",
            [](problem_dto::list_filter& filter_value,
               std::string_view key,
               std::string_view raw_value) -> std::expected<void, dto_validation_error> {
                return query_param_util::parse_unique_query_param(
                    filter_value.title_opt,
                    key,
                    raw_value,
                    [](std::string_view value) -> std::optional<std::string> {
                        if(value.empty()){
                            return std::nullopt;
                        }

                        return std::string{value};
                    }
                );
            }
        },
        {
            "state",
            [](problem_dto::list_filter& filter_value,
               std::string_view key,
               std::string_view raw_value) -> std::expected<void, dto_validation_error> {
                return query_param_util::parse_unique_query_param(
                    filter_value.state_opt,
                    key,
                    raw_value,
                    parse_problem_list_state
                );
            }
        },
        {
            "sort",
            [](problem_dto::list_filter& filter_value,
               std::string_view key,
               std::string_view raw_value) -> std::expected<void, dto_validation_error> {
                return query_param_util::parse_unique_query_param(
                    filter_value.sort_opt,
                    key,
                    raw_value,
                    parse_problem_list_sort
                );
            }
        },
        {
            "direction",
            [](problem_dto::list_filter& filter_value,
               std::string_view key,
               std::string_view raw_value) -> std::expected<void, dto_validation_error> {
                return query_param_util::parse_unique_query_param(
                    filter_value.direction_opt,
                    key,
                    raw_value,
                    parse_sort_direction
                );
            }
        },
        {
            "limit",
            [](problem_dto::list_filter& filter_value,
               std::string_view key,
               std::string_view raw_value) -> std::expected<void, dto_validation_error> {
                return query_param_util::parse_unique_query_param(
                    filter_value.limit_opt,
                    key,
                    raw_value,
                    string_util::parse_positive_int32
                );
            }
        },
        {
            "offset",
            [](problem_dto::list_filter& filter_value,
               std::string_view key,
               std::string_view raw_value) -> std::expected<void, dto_validation_error> {
                return query_param_util::parse_unique_query_param(
                    filter_value.offset_opt,
                    key,
                    raw_value,
                    parse_non_negative_int64
                );
            }
        }
    }};
}

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
    return query_param_util::make_filter_from_query_params(
        query_params,
        problem_list_filter_bindings
    );
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

std::expected<problem_dto::testcase_move_request, dto_validation_error>
problem_dto::make_testcase_move_request_from_json(const boost::json::object& json){
    const auto source_testcase_order_opt = http_util::get_positive_int32_field(
        json,
        "source_testcase_order"
    );
    if(!source_testcase_order_opt){
        return std::unexpected(dto_validation_error{
            .code = "invalid_field",
            .message = "source_testcase_order must be a positive integer",
            .field_opt = "source_testcase_order"
        });
    }

    const auto target_testcase_order_opt = http_util::get_positive_int32_field(
        json,
        "target_testcase_order"
    );
    if(!target_testcase_order_opt){
        return std::unexpected(dto_validation_error{
            .code = "invalid_field",
            .message = "target_testcase_order must be a positive integer",
            .field_opt = "target_testcase_order"
        });
    }

    testcase_move_request testcase_move_request_value;
    testcase_move_request_value.source_testcase_order = *source_testcase_order_opt;
    testcase_move_request_value.target_testcase_order = *target_testcase_order_opt;
    return testcase_move_request_value;
}

problem_dto::summary problem_dto::make_summary_from_row(
    const pqxx::row& problem_summary_row
){
    summary summary_value;
    summary_value.problem_id = problem_summary_row[0].as<std::int64_t>();
    summary_value.title = problem_summary_row[1].as<std::string>();
    summary_value.version = problem_summary_row[2].as<std::int32_t>();
    summary_value.time_limit_ms = problem_summary_row[3].as<std::int32_t>();
    summary_value.memory_limit_mb = problem_summary_row[4].as<std::int32_t>();
    summary_value.submission_count = problem_summary_row[5].as<std::int64_t>();
    summary_value.accepted_count = problem_summary_row[6].as<std::int64_t>();
    if(!problem_summary_row[7].is_null()){
        summary_value.user_problem_state_opt = problem_summary_row[7].as<std::string>();
    }
    return summary_value;
}

std::vector<problem_dto::summary> problem_dto::make_summary_list_from_result(
    const pqxx::result& problem_summary_result
){
    std::vector<summary> summary_values;
    summary_values.reserve(problem_summary_result.size());
    for(const auto& problem_summary_row : problem_summary_result){
        summary_values.push_back(make_summary_from_row(problem_summary_row));
    }
    return summary_values;
}
