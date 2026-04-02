#include "common/query_param_util.hpp"

http_error query_param_util::make_duplicate_query_parameter_error(
    std::string_view key
){
    return http_error{
        http_error_code::duplicate_query_parameter,
        "duplicate query parameter: " + std::string{key},
        std::string{key}
    };
}

http_error query_param_util::make_invalid_query_parameter_error(
    std::string_view key
){
    return http_error{
        http_error_code::invalid_query_parameter,
        "invalid query parameter: " + std::string{key},
        std::string{key}
    };
}

http_error query_param_util::make_unsupported_query_parameter_error(
    std::string_view key
){
    return http_error{
        http_error_code::unsupported_query_parameter,
        "unsupported query parameter: " + std::string{key},
        std::string{key}
    };
}
