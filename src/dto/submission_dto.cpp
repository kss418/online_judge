#include "dto/submission_dto.hpp"

#include "http_server/http_util.hpp"

std::optional<submission_dto::source> submission_dto::make_source_from_json(
    const boost::json::object& json
){
    const auto language_opt = http_util::get_non_empty_string_field(
        json,
        "language"
    );
    const auto source_code_opt = http_util::get_non_empty_string_field(
        json,
        "source_code"
    );
    if(!language_opt || !source_code_opt){
        return std::nullopt;
    }

    source source_value;
    source_value.language = std::string{*language_opt};
    source_value.source_code = std::string{*source_code_opt};
    return source_value;
}

std::optional<submission_dto::create_request> submission_dto::make_create_request_from_json(
    const boost::json::object& json,
    std::int64_t user_id,
    std::int64_t problem_id
){
    if(user_id <= 0 || problem_id <= 0){
        return std::nullopt;
    }

    const auto source_opt = make_source_from_json(json);
    if(!source_opt){
        return std::nullopt;
    }

    create_request create_request_value;
    create_request_value.user_id = user_id;
    create_request_value.problem_id = problem_id;
    create_request_value.source_value = std::move(*source_opt);
    return create_request_value;
}
