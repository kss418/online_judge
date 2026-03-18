#include "dto/submission_dto.hpp"

#include "http_server/http_util.hpp"

std::optional<submission_dto::source> submission_dto::make_source(
    const boost::json::object& request_object
){
    const auto language_opt = http_util::get_non_empty_string_field(
        request_object,
        "language"
    );
    const auto source_code_opt = http_util::get_non_empty_string_field(
        request_object,
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
