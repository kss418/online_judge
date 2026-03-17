#include "common/string_util.hpp"

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
