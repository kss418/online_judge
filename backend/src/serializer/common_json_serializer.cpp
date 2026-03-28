#include "serializer/common_json_serializer.hpp"

#include <utility>

boost::json::object common_json_serializer::make_error_object(
    std::string_view code,
    std::string_view message,
    std::optional<std::string> field_opt
){
    boost::json::object error_object;
    error_object["code"] = code;
    error_object["message"] = message;
    if(field_opt){
        error_object["field"] = *field_opt;
    }

    boost::json::object response_object;
    response_object["error"] = std::move(error_object);
    return response_object;
}

boost::json::object common_json_serializer::make_message_object(std::string_view message){
    boost::json::object response_object;
    response_object["message"] = message;
    return response_object;
}
