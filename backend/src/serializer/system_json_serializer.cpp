#include "serializer/system_json_serializer.hpp"

#include <cstdint>

namespace{
    boost::json::object make_supported_language_object(
        const language_util::supported_language& supported_language_value
    ){
        boost::json::object response_object;
        response_object["language"] = supported_language_value.language;
        response_object["source_extension"] = supported_language_value.source_extension;
        return response_object;
    }

    boost::json::array make_supported_language_array(
        std::span<const language_util::supported_language> supported_language_values
    ){
        boost::json::array response_array;
        response_array.reserve(supported_language_values.size());
        for(const auto& supported_language_value : supported_language_values){
            response_array.push_back(make_supported_language_object(supported_language_value));
        }
        return response_array;
    }
}

boost::json::object system_json_serializer::make_supported_language_list_object(
    std::span<const language_util::supported_language> supported_language_values
){
    boost::json::object response_object;
    response_object["language_count"] =
        static_cast<std::int64_t>(supported_language_values.size());
    response_object["languages"] = make_supported_language_array(supported_language_values);
    return response_object;
}
