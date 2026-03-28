#pragma once

#include <boost/json.hpp>

#include <optional>
#include <string>
#include <string_view>

namespace common_json_serializer{
    boost::json::object make_error_object(
        std::string_view code,
        std::string_view message,
        std::optional<std::string> field_opt = std::nullopt
    );

    boost::json::object make_message_object(std::string_view message);
}
