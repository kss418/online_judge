#pragma once

#include "common/language_util.hpp"

#include <boost/json.hpp>

#include <span>

namespace system_json_serializer{
    boost::json::object make_supported_language_list_object(
        std::span<const language_util::supported_language> supported_language_values
    );
}
