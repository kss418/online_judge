#include "common/query_param_util.hpp"

dto_validation_error query_param_util::make_duplicate_query_parameter_error(
    std::string_view key
){
    return dto_validation_error{
        .code = "duplicate_query_parameter",
        .message = "duplicate query parameter: " + std::string{key},
        .field_opt = std::string{key}
    };
}

dto_validation_error query_param_util::make_invalid_query_parameter_error(
    std::string_view key
){
    return dto_validation_error{
        .code = "invalid_query_parameter",
        .message = "invalid query parameter: " + std::string{key},
        .field_opt = std::string{key}
    };
}

dto_validation_error query_param_util::make_unsupported_query_parameter_error(
    std::string_view key
){
    return dto_validation_error{
        .code = "unsupported_query_parameter",
        .message = "unsupported query parameter: " + std::string{key},
        .field_opt = std::string{key}
    };
}
