#include "request_parser/submission_request_parser.hpp"

#include "common/json_field_util.hpp"
#include "common/language_util.hpp"
#include "common/query_param_util.hpp"
#include "common/string_util.hpp"
#include "error/request_error.hpp"

#include <array>
#include <limits>
#include <optional>
#include <string>
#include <utility>

namespace{
    std::optional<std::int64_t> parse_positive_json_int64(const boost::json::value& value){
        if(value.is_int64()){
            const std::int64_t int64_value = value.as_int64();
            if(int64_value <= 0){
                return std::nullopt;
            }

            return int64_value;
        }

        if(value.is_uint64()){
            const std::uint64_t uint64_value = value.as_uint64();
            if(
                uint64_value == 0 ||
                uint64_value > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())
            ){
                return std::nullopt;
            }

            return static_cast<std::int64_t>(uint64_value);
        }

        return std::nullopt;
    }

    const std::array<
        query_param_util::query_param_binding<submission_dto::list_filter>,
        7
    > submission_list_filter_bindings{{
        {
            "user_id",
            [](submission_dto::list_filter& filter_value,
               std::string_view key,
               std::string_view raw_value) -> std::expected<void, http_error> {
                return query_param_util::parse_unique_query_param(
                    filter_value.user_id_opt,
                    key,
                    raw_value,
                    string_util::parse_positive_int64,
                    request_error::query_param_error_adapter::duplicate,
                    request_error::query_param_error_adapter::invalid
                );
            }
        },
        {
            "user_login_id",
            [](submission_dto::list_filter& filter_value,
               std::string_view key,
               std::string_view raw_value) -> std::expected<void, http_error> {
                return query_param_util::parse_unique_query_param(
                    filter_value.user_login_id_opt,
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
            "problem_id",
            [](submission_dto::list_filter& filter_value,
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
            "language",
            [](submission_dto::list_filter& filter_value,
               std::string_view key,
               std::string_view raw_value) -> std::expected<void, http_error> {
                return query_param_util::parse_unique_query_param(
                    filter_value.language_opt,
                    key,
                    raw_value,
                    [](std::string_view value) -> std::optional<std::string> {
                        const auto language_opt = language_util::find_supported_language(value);
                        if(!language_opt){
                            return std::nullopt;
                        }

                        return std::string{language_opt->language};
                    },
                    request_error::query_param_error_adapter::duplicate,
                    request_error::query_param_error_adapter::invalid
                );
            }
        },
        {
            "status",
            [](submission_dto::list_filter& filter_value,
               std::string_view key,
               std::string_view raw_value) -> std::expected<void, http_error> {
                return query_param_util::parse_unique_query_param(
                    filter_value.status_opt,
                    key,
                    raw_value,
                    [](std::string_view value) -> std::optional<std::string> {
                        const auto status_opt = parse_submission_status(value);
                        if(!status_opt){
                            return std::nullopt;
                        }

                        return to_string(*status_opt);
                    },
                    request_error::query_param_error_adapter::duplicate,
                    request_error::query_param_error_adapter::invalid
                );
            }
        },
        {
            "limit",
            [](submission_dto::list_filter& filter_value,
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
            "before_submission_id",
            [](submission_dto::list_filter& filter_value,
               std::string_view key,
               std::string_view raw_value) -> std::expected<void, http_error> {
                return query_param_util::parse_unique_query_param(
                    filter_value.before_submission_id_opt,
                    key,
                    raw_value,
                    string_util::parse_positive_int64,
                    request_error::query_param_error_adapter::duplicate,
                    request_error::query_param_error_adapter::invalid
                );
            }
        }
    }};
}

std::expected<submission_dto::source, http_error>
submission_request_parser::parse_source(const boost::json::object& json){
    const auto language_opt = json_field_util::get_non_empty_string_field(
        json,
        "language"
    );
    if(!language_opt){
        return std::unexpected(request_error::make_missing_field_error("language"));
    }
    if(!language_util::find_supported_language(*language_opt)){
        return std::unexpected(request_error::make_invalid_field_error(
            "language",
            "unsupported language: " + std::string{*language_opt}
        ));
    }

    const auto source_code_opt = json_field_util::get_non_empty_string_field(
        json,
        "source_code"
    );
    if(!source_code_opt){
        return std::unexpected(request_error::make_missing_field_error("source_code"));
    }

    submission_dto::source source_value;
    source_value.language = std::string{*language_opt};
    source_value.source_code = std::string{*source_code_opt};
    return source_value;
}

std::expected<submission_dto::status_batch_request, http_error>
submission_request_parser::parse_status_batch_request(const boost::json::object& json){
    const auto* submission_ids_value = json.if_contains("submission_ids");
    if(submission_ids_value == nullptr){
        return std::unexpected(request_error::make_missing_field_error("submission_ids"));
    }
    if(!submission_ids_value->is_array()){
        return std::unexpected(request_error::make_invalid_field_error(
            "submission_ids",
            "submission_ids must be an array of positive integers"
        ));
    }

    submission_dto::status_batch_request request_value;
    const auto& submission_ids = submission_ids_value->as_array();
    request_value.submission_ids.reserve(submission_ids.size());
    for(const auto& submission_id_value : submission_ids){
        const auto submission_id_opt = parse_positive_json_int64(submission_id_value);
        if(!submission_id_opt){
            return std::unexpected(request_error::make_invalid_field_error(
                "submission_ids",
                "submission_ids must be an array of positive integers"
            ));
        }

        request_value.submission_ids.push_back(*submission_id_opt);
    }

    return request_value;
}

std::expected<submission_dto::list_filter, http_error>
submission_request_parser::parse_list_filter(const std::vector<query_param>& query_params){
    return query_param_util::make_filter_from_query_params(
        query_params,
        submission_list_filter_bindings,
        request_error::query_param_error_adapter::unsupported
    );
}

std::expected<submission_dto::create_request, http_error>
submission_request_parser::parse_create_request(
    const boost::json::object& json,
    std::int64_t user_id,
    std::int64_t problem_id
){
    if(user_id <= 0){
        return std::unexpected(request_error::make_invalid_argument_error(
            "user_id must be positive",
            "user_id"
        ));
    }
    if(problem_id <= 0){
        return std::unexpected(request_error::make_invalid_argument_error(
            "problem_id must be positive",
            "problem_id"
        ));
    }

    const auto source_exp = parse_source(json);
    if(!source_exp){
        return std::unexpected(source_exp.error());
    }

    submission_dto::create_request create_request_value;
    create_request_value.user_id = user_id;
    create_request_value.problem_id = problem_id;
    create_request_value.source_value = std::move(*source_exp);
    return create_request_value;
}
