#include "common/json_field_util.hpp"

#include <limits>

std::optional<std::string_view> json_field_util::get_string_field(
    const boost::json::object& object,
    std::string_view key
){
    const auto* value = object.if_contains(key);
    if(!value || !value->is_string()){
        return std::nullopt;
    }

    const auto& string_value = value->as_string();
    return std::string_view{string_value.data(), string_value.size()};
}

std::optional<std::string_view> json_field_util::get_non_empty_string_field(
    const boost::json::object& object,
    std::string_view key
){
    const auto string_value_opt = get_string_field(object, key);
    if(!string_value_opt || string_value_opt->empty()){
        return std::nullopt;
    }

    return string_value_opt;
}

std::optional<std::int64_t> json_field_util::get_positive_int64_field(
    const boost::json::object& object,
    std::string_view key
){
    const auto* value = object.if_contains(key);
    if(value == nullptr){
        return std::nullopt;
    }

    if(value->is_int64()){
        const std::int64_t int64_value = value->as_int64();
        if(int64_value <= 0){
            return std::nullopt;
        }

        return int64_value;
    }

    if(value->is_uint64()){
        const std::uint64_t uint64_value = value->as_uint64();
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

std::optional<std::int32_t> json_field_util::get_positive_int32_field(
    const boost::json::object& object,
    std::string_view key
){
    const auto int64_value_opt = get_positive_int64_field(object, key);
    if(
        !int64_value_opt ||
        *int64_value_opt > std::numeric_limits<std::int32_t>::max()
    ){
        return std::nullopt;
    }

    return static_cast<std::int32_t>(*int64_value_opt);
}
