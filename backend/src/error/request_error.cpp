#include "error/request_error.hpp"

#include <array>
#include <cstddef>
#include <string_view>
#include <utility>

namespace{
    enum class request_error_message_template_kind{
        http_error_default,
        append_field_value
    };

    struct request_error_message_template{
        request_error_message_template_kind kind =
            request_error_message_template_kind::http_error_default;
        std::string_view prefix;
    };

    constexpr std::array request_error_http_error_codes{
        http_error_code::bad_request,
        http_error_code::payload_too_large,
        http_error_code::invalid_json,
        http_error_code::invalid_query_string,
        http_error_code::duplicate_query_parameter,
        http_error_code::invalid_query_parameter,
        http_error_code::unsupported_query_parameter,
        http_error_code::missing_field,
        http_error_code::invalid_field,
        http_error_code::invalid_length,
        http_error_code::invalid_argument,
    };

    constexpr request_error_message_template http_error_default_template{};

    constexpr std::array request_error_message_templates{
        http_error_default_template,
        http_error_default_template,
        http_error_default_template,
        http_error_default_template,
        request_error_message_template{
            request_error_message_template_kind::append_field_value,
            "duplicate query parameter"
        },
        request_error_message_template{
            request_error_message_template_kind::append_field_value,
            "invalid query parameter"
        },
        request_error_message_template{
            request_error_message_template_kind::append_field_value,
            "unsupported query parameter"
        },
        request_error_message_template{
            request_error_message_template_kind::append_field_value,
            "required field"
        },
        request_error_message_template{
            request_error_message_template_kind::append_field_value,
            "invalid field"
        },
        request_error_message_template{
            request_error_message_template_kind::append_field_value,
            "invalid length"
        },
        http_error_default_template,
    };

    const request_error_message_template& describe_request_error_message_template(
        request_error_code code
    ){
        static constexpr request_error_message_template unknown_template{};

        const auto index = static_cast<std::size_t>(code);
        if(index >= request_error_message_templates.size()){
            return unknown_template;
        }

        return request_error_message_templates[index];
    }

    http_error_code to_http_error_code(request_error_code code){
        const auto index = static_cast<std::size_t>(code);
        if(index >= request_error_http_error_codes.size()){
            return http_error_code::bad_request;
        }

        return request_error_http_error_codes[index];
    }

    std::string make_prefixed_message(
        std::string_view prefix,
        std::string_view field_value
    ){
        return std::string{prefix} + ": " + std::string{field_value};
    }

    std::string resolve_message(
        request_error_code code,
        std::string message,
        const std::optional<std::string>& field_opt
    ){
        if(!message.empty()){
            return message;
        }

        const auto& message_template = describe_request_error_message_template(code);
        if(
            message_template.kind !=
                request_error_message_template_kind::append_field_value ||
            !field_opt.has_value()
        ){
            return {};
        }

        return make_prefixed_message(message_template.prefix, *field_opt);
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
        resolve_message(code, std::move(message), field_opt),
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
        {},
        std::string{key}
    );
}

http_error request_error::make_invalid_query_parameter_error(
    std::string_view key,
    std::string message
){
    if(message.empty()){
        return make_error(
            request_error_code::invalid_query_parameter,
            {},
            std::string{key}
        );
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
        {},
        std::string{key}
    );
}

http_error request_error::make_missing_field_error(std::string_view field_name){
    return make_error(
        request_error_code::missing_field,
        {},
        std::string{field_name}
    );
}

http_error request_error::make_invalid_field_error(
    std::string_view field_name,
    std::string message
){
    if(message.empty()){
        return make_error(
            request_error_code::invalid_field,
            {},
            std::string{field_name}
        );
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
        return make_error(
            request_error_code::invalid_length,
            {},
            std::string{field_name}
        );
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
