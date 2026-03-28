#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace string_util{
    std::string_view trim_left_whitespace(std::string_view value);
    std::string_view trim_right_whitespace(std::string_view value);
    std::optional<std::int32_t> parse_positive_int32(std::string_view value);
    std::optional<std::int64_t> parse_positive_int64(std::string_view value);
    std::optional<std::string> decode_percent_encoded(std::string_view value);
}
