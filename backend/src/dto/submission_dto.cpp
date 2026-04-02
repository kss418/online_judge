#include "dto/submission_dto.hpp"

#include "common/language_util.hpp"
#include "common/query_param_util.hpp"
#include "common/string_util.hpp"
#include "common/json_field_util.hpp"

#include <limits>

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
                    string_util::parse_positive_int64
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
                    }
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
                    string_util::parse_positive_int64
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
                    }
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
                    }
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
                    string_util::parse_positive_int32
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
                    string_util::parse_positive_int64
                );
            }
        }
    }};
}

bool submission_dto::is_valid(const source& source_value){
    return
        !source_value.language.empty() &&
        !source_value.source_code.empty() &&
        language_util::find_supported_language(source_value.language).has_value();
}

bool submission_dto::is_valid(const create_request& create_request_value){
    return
        create_request_value.user_id > 0 &&
        create_request_value.problem_id > 0 &&
        is_valid(create_request_value.source_value);
}

bool submission_dto::is_valid(const lease_request& lease_request_value){
    return lease_request_value.lease_duration > std::chrono::seconds::zero();
}

bool submission_dto::is_valid(const status_update& status_update_value){
    return status_update_value.submission_id > 0;
}

bool submission_dto::is_valid(const finalize_request& finalize_request_value){
    return finalize_request_value.submission_id > 0;
}

std::expected<submission_dto::source, http_error> submission_dto::make_source_from_json(
    const boost::json::object& json
){
    const auto language_opt = json_field_util::get_non_empty_string_field(
        json,
        "language"
    );
    if(!language_opt){
        return std::unexpected(http_error{
            http_error_code::missing_field,
            "required field: language",
            "language"
        });
    }
    if(!language_util::find_supported_language(*language_opt)){
        return std::unexpected(http_error{
            http_error_code::invalid_field,
            "unsupported language: " + std::string{*language_opt},
            "language"
        });
    }

    const auto source_code_opt = json_field_util::get_non_empty_string_field(
        json,
        "source_code"
    );
    if(!source_code_opt){
        return std::unexpected(http_error{
            http_error_code::missing_field,
            "required field: source_code",
            "source_code"
        });
    }

    source source_value;
    source_value.language = std::string{*language_opt};
    source_value.source_code = std::string{*source_code_opt};
    return source_value;
}

std::expected<submission_dto::list_filter, http_error>
submission_dto::make_list_filter_from_query_params(
    const std::vector<query_param>& query_params
){
    return query_param_util::make_filter_from_query_params(
        query_params,
        submission_list_filter_bindings
    );
}

std::expected<submission_dto::status_batch_request, http_error>
submission_dto::make_status_batch_request_from_json(
    const boost::json::object& json
){
    const auto* submission_ids_value = json.if_contains("submission_ids");
    if(submission_ids_value == nullptr){
        return std::unexpected(http_error{
            http_error_code::missing_field,
            "required field: submission_ids",
            "submission_ids"
        });
    }
    if(!submission_ids_value->is_array()){
        return std::unexpected(http_error{
            http_error_code::invalid_field,
            "submission_ids must be an array of positive integers",
            "submission_ids"
        });
    }

    status_batch_request request_value;
    const auto& submission_ids = submission_ids_value->as_array();
    request_value.submission_ids.reserve(submission_ids.size());
    for(const auto& submission_id_value : submission_ids){
        const auto submission_id_opt = parse_positive_json_int64(submission_id_value);
        if(!submission_id_opt){
            return std::unexpected(http_error{
                http_error_code::invalid_field,
                "submission_ids must be an array of positive integers",
                "submission_ids"
            });
        }

        request_value.submission_ids.push_back(*submission_id_opt);
    }

    return request_value;
}

std::expected<submission_dto::create_request, http_error>
submission_dto::make_create_request_from_json(
    const boost::json::object& json,
    std::int64_t user_id,
    std::int64_t problem_id
){
    if(user_id <= 0){
        return std::unexpected(http_error{
            http_error_code::invalid_argument,
            "user_id must be positive",
            "user_id"
        });
    }
    if(problem_id <= 0){
        return std::unexpected(http_error{
            http_error_code::invalid_argument,
            "problem_id must be positive",
            "problem_id"
        });
    }

    const auto source_exp = make_source_from_json(json);
    if(!source_exp){
        return std::unexpected(source_exp.error());
    }

    create_request create_request_value;
    create_request_value.user_id = user_id;
    create_request_value.problem_id = problem_id;
    create_request_value.source_value = std::move(*source_exp);
    return create_request_value;
}

submission_dto::created submission_dto::make_created(
    std::int64_t submission_id,
    submission_status submission_status_value
){
    created created_value;
    created_value.submission_id = submission_id;
    created_value.status = to_string(submission_status_value);
    return created_value;
}

std::optional<submission_status> submission_dto::make_submission_status(
    std::string_view submission_status_string
){
    return parse_submission_status(submission_status_string);
}

submission_dto::status_update submission_dto::make_status_update(
    std::int64_t submission_id,
    submission_status to_status,
    std::optional<std::string> reason_opt
){
    status_update status_update_value;
    status_update_value.submission_id = submission_id;
    status_update_value.to_status = to_status;
    status_update_value.reason_opt = std::move(reason_opt);
    return status_update_value;
}

submission_dto::finalize_request submission_dto::make_finalize_request(
    std::int64_t submission_id,
    submission_status to_status,
    std::optional<std::int16_t> score_opt,
    std::optional<std::string> compile_output_opt,
    std::optional<std::string> judge_output_opt,
    std::optional<std::int64_t> elapsed_ms_opt,
    std::optional<std::int64_t> max_rss_kb_opt,
    std::optional<std::string> reason_opt
){
    finalize_request finalize_request_value;
    finalize_request_value.submission_id = submission_id;
    finalize_request_value.to_status = to_status;
    finalize_request_value.score_opt = std::move(score_opt);
    finalize_request_value.compile_output_opt = std::move(compile_output_opt);
    finalize_request_value.judge_output_opt = std::move(judge_output_opt);
    finalize_request_value.elapsed_ms_opt = std::move(elapsed_ms_opt);
    finalize_request_value.max_rss_kb_opt = std::move(max_rss_kb_opt);
    finalize_request_value.reason_opt = std::move(reason_opt);
    return finalize_request_value;
}

submission_dto::finalize_result submission_dto::make_finalize_result(
    std::int64_t problem_id,
    bool should_increase_accepted_count
){
    finalize_result finalize_result_value;
    finalize_result_value.problem_id = problem_id;
    finalize_result_value.should_increase_accepted_count = should_increase_accepted_count;
    return finalize_result_value;
}
