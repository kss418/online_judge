#include "dto/submission_dto.hpp"

#include "common/string_util.hpp"
#include "http_server/http_util.hpp"

std::expected<submission_dto::source, dto_validation_error> submission_dto::make_source_from_json(
    const boost::json::object& json
){
    const auto language_opt = http_util::get_non_empty_string_field(
        json,
        "language"
    );
    if(!language_opt){
        return std::unexpected(dto_validation_error{
            .code = "missing_field",
            .message = "required field: language",
            .field_opt = "language"
        });
    }

    const auto source_code_opt = http_util::get_non_empty_string_field(
        json,
        "source_code"
    );
    if(!source_code_opt){
        return std::unexpected(dto_validation_error{
            .code = "missing_field",
            .message = "required field: source_code",
            .field_opt = "source_code"
        });
    }

    source source_value;
    source_value.language = std::string{*language_opt};
    source_value.source_code = std::string{*source_code_opt};
    return source_value;
}

std::expected<submission_dto::list_filter, dto_validation_error>
submission_dto::make_list_filter_from_query_params(
    const std::vector<http_util::query_param>& query_params
){
    list_filter filter_value;
    for(const auto& query_param : query_params){
        if(query_param.key == "top"){
            if(filter_value.top_submission_id_opt){
                return std::unexpected(dto_validation_error{
                    .code = "duplicate_query_parameter",
                    .message = "duplicate query parameter: top",
                    .field_opt = "top"
                });
            }

            const auto top_submission_id_opt = string_util::parse_positive_int64(query_param.value);
            if(!top_submission_id_opt){
                return std::unexpected(dto_validation_error{
                    .code = "invalid_query_parameter",
                    .message = "invalid query parameter: top",
                    .field_opt = "top"
                });
            }

            filter_value.top_submission_id_opt = *top_submission_id_opt;
            continue;
        }
        if(query_param.key == "user_id"){
            if(filter_value.user_id_opt){
                return std::unexpected(dto_validation_error{
                    .code = "duplicate_query_parameter",
                    .message = "duplicate query parameter: user_id",
                    .field_opt = "user_id"
                });
            }

            const auto user_id_opt = string_util::parse_positive_int64(query_param.value);
            if(!user_id_opt){
                return std::unexpected(dto_validation_error{
                    .code = "invalid_query_parameter",
                    .message = "invalid query parameter: user_id",
                    .field_opt = "user_id"
                });
            }

            filter_value.user_id_opt = *user_id_opt;
            continue;
        }
        if(query_param.key == "problem_id"){
            if(filter_value.problem_id_opt){
                return std::unexpected(dto_validation_error{
                    .code = "duplicate_query_parameter",
                    .message = "duplicate query parameter: problem_id",
                    .field_opt = "problem_id"
                });
            }

            const auto problem_id_opt = string_util::parse_positive_int64(query_param.value);
            if(!problem_id_opt){
                return std::unexpected(dto_validation_error{
                    .code = "invalid_query_parameter",
                    .message = "invalid query parameter: problem_id",
                    .field_opt = "problem_id"
                });
            }

            filter_value.problem_id_opt = *problem_id_opt;
            continue;
        }
        if(query_param.key == "status"){
            if(filter_value.status_opt){
                return std::unexpected(dto_validation_error{
                    .code = "duplicate_query_parameter",
                    .message = "duplicate query parameter: status",
                    .field_opt = "status"
                });
            }

            const auto status_opt = parse_submission_status(query_param.value);
            if(!status_opt){
                return std::unexpected(dto_validation_error{
                    .code = "invalid_query_parameter",
                    .message = "invalid query parameter: status",
                    .field_opt = "status"
                });
            }

            filter_value.status_opt = to_string(*status_opt);
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

std::expected<submission_dto::create_request, dto_validation_error>
submission_dto::make_create_request_from_json(
    const boost::json::object& json,
    std::int64_t user_id,
    std::int64_t problem_id
){
    if(user_id <= 0){
        return std::unexpected(dto_validation_error{
            .code = "invalid_argument",
            .message = "user_id must be positive",
            .field_opt = "user_id"
        });
    }
    if(problem_id <= 0){
        return std::unexpected(dto_validation_error{
            .code = "invalid_argument",
            .message = "problem_id must be positive",
            .field_opt = "problem_id"
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
