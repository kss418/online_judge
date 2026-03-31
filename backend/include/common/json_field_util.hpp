#pragma once

#include <boost/json.hpp>

#include <cstdint>
#include <optional>
#include <string_view>

namespace json_field_util{
    std::optional<std::string_view> get_string_field(
        const boost::json::object& object,
        std::string_view key
    );

    std::optional<std::string_view> get_non_empty_string_field(
        const boost::json::object& object,
        std::string_view key
    );

    std::optional<std::int64_t> get_positive_int64_field(
        const boost::json::object& object,
        std::string_view key
    );

    std::optional<std::int32_t> get_positive_int32_field(
        const boost::json::object& object,
        std::string_view key
    );
}
