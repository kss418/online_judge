#include "common/string_util.hpp"

#include <charconv>
#include <cctype>

static bool is_whitespace(char character){
    return std::isspace(static_cast<unsigned char>(character)) != 0;
}

std::string_view string_util::trim_left_whitespace(std::string_view value){
    while(!value.empty() && is_whitespace(value.front())){
        value.remove_prefix(1);
    }

    return value;
}

std::string_view string_util::trim_right_whitespace(std::string_view value){
    while(!value.empty() && is_whitespace(value.back())){
        value.remove_suffix(1);
    }

    return value;
}

std::optional<std::int64_t> string_util::parse_positive_int64(std::string_view value){
    std::int64_t int64_value = 0;
    const auto* begin = value.data();
    const auto* end = value.data() + value.size();
    const auto from_chars_result = std::from_chars(begin, end, int64_value);
    if(
        from_chars_result.ec != std::errc{} ||
        from_chars_result.ptr != end ||
        int64_value <= 0
    ){
        return std::nullopt;
    }

    return int64_value;
}
