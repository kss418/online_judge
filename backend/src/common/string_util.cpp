#include "common/string_util.hpp"

#include <charconv>
#include <cctype>
#include <limits>
#include <string>

namespace{
    std::optional<unsigned char> parse_hex_digit(char character){
        if(character >= '0' && character <= '9'){
            return static_cast<unsigned char>(character - '0');
        }

        if(character >= 'a' && character <= 'f'){
            return static_cast<unsigned char>(character - 'a' + 10);
        }

        if(character >= 'A' && character <= 'F'){
            return static_cast<unsigned char>(character - 'A' + 10);
        }

        return std::nullopt;
    }
}

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

std::optional<std::int32_t> string_util::parse_positive_int32(std::string_view value){
    const auto int64_value_opt = parse_positive_int64(value);
    if(
        !int64_value_opt ||
        *int64_value_opt > std::numeric_limits<std::int32_t>::max()
    ){
        return std::nullopt;
    }

    return static_cast<std::int32_t>(*int64_value_opt);
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

std::optional<std::string> string_util::decode_percent_encoded(std::string_view value){
    std::string decoded_value;
    decoded_value.reserve(value.size());

    for(std::size_t index = 0; index < value.size(); ++index){
        if(value[index] != '%'){
            decoded_value.push_back(value[index]);
            continue;
        }

        if(index + 2 >= value.size()){
            return std::nullopt;
        }

        const auto high_nibble_opt = parse_hex_digit(value[index + 1]);
        const auto low_nibble_opt = parse_hex_digit(value[index + 2]);
        if(!high_nibble_opt || !low_nibble_opt){
            return std::nullopt;
        }

        const unsigned char decoded_character =
            static_cast<unsigned char>((*high_nibble_opt << 4) | *low_nibble_opt);
        decoded_value.push_back(static_cast<char>(decoded_character));
        index += 2;
    }

    return decoded_value;
}
