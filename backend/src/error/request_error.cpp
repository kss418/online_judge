#include "error/request_error.hpp"

#include <utility>

namespace{
    http_error_code to_http_error_code(request_error_code code){
        switch(code){
            case request_error_code::bad_request:
                return http_error_code::bad_request;
            case request_error_code::payload_too_large:
                return http_error_code::payload_too_large;
            case request_error_code::invalid_json:
                return http_error_code::invalid_json;
            case request_error_code::invalid_query_string:
                return http_error_code::invalid_query_string;
            case request_error_code::duplicate_query_parameter:
                return http_error_code::duplicate_query_parameter;
            case request_error_code::invalid_query_parameter:
                return http_error_code::invalid_query_parameter;
            case request_error_code::unsupported_query_parameter:
                return http_error_code::unsupported_query_parameter;
            case request_error_code::missing_field:
                return http_error_code::missing_field;
            case request_error_code::invalid_field:
                return http_error_code::invalid_field;
            case request_error_code::invalid_length:
                return http_error_code::invalid_length;
            case request_error_code::invalid_argument:
                return http_error_code::invalid_argument;
        }

        return http_error_code::bad_request;
    }
}

http_error request_error::query_param_error_adapter::duplicate(std::string_view key){
    return make_duplicate_query_parameter_error(key);
}

http_error request_error::query_param_error_adapter::invalid(std::string_view key){
    return make_invalid_query_parameter_error(key);
}

http_error request_error::query_param_error_adapter::unsupported(std::string_view key){
    return make_unsupported_query_parameter_error(key);
}

http_error request_error::make_error(
    request_error_code code,
    std::string message,
    std::optional<std::string> field_opt
){
    return http_error{
        to_http_error_code(code),
        std::move(message),
        std::move(field_opt)
    };
}

http_error request_error::make_bad_request_error(std::string message){
    return make_error(request_error_code::bad_request, std::move(message));
}

http_error request_error::make_payload_too_large_error(){
    return make_error(request_error_code::payload_too_large);
}

http_error request_error::make_invalid_json_error(){
    return make_error(request_error_code::invalid_json);
}

http_error request_error::make_invalid_query_string_error(){
    return make_error(request_error_code::invalid_query_string);
}

http_error request_error::make_duplicate_query_parameter_error(std::string_view key){
    return make_error(
        request_error_code::duplicate_query_parameter,
        "duplicate query parameter: " + std::string{key},
        std::string{key}
    );
}

http_error request_error::make_invalid_query_parameter_error(
    std::string_view key,
    std::string message
){
    if(message.empty()){
        message = "invalid query parameter: " + std::string{key};
    }

    return make_error(
        request_error_code::invalid_query_parameter,
        std::move(message),
        std::string{key}
    );
}

http_error request_error::make_unsupported_query_parameter_error(std::string_view key){
    return make_error(
        request_error_code::unsupported_query_parameter,
        "unsupported query parameter: " + std::string{key},
        std::string{key}
    );
}

http_error request_error::make_missing_field_error(std::string_view field_name){
    return make_error(
        request_error_code::missing_field,
        "required field: " + std::string{field_name},
        std::string{field_name}
    );
}

http_error request_error::make_invalid_field_error(
    std::string_view field_name,
    std::string message
){
    if(message.empty()){
        message = "invalid field: " + std::string{field_name};
    }

    return make_error(
        request_error_code::invalid_field,
        std::move(message),
        std::string{field_name}
    );
}

http_error request_error::make_invalid_length_error(
    std::string_view field_name,
    std::string message
){
    if(message.empty()){
        message = "invalid length: " + std::string{field_name};
    }

    return make_error(
        request_error_code::invalid_length,
        std::move(message),
        std::string{field_name}
    );
}

http_error request_error::make_invalid_argument_error(
    std::string message,
    std::optional<std::string> field_opt
){
    return make_error(
        request_error_code::invalid_argument,
        std::move(message),
        std::move(field_opt)
    );
}
