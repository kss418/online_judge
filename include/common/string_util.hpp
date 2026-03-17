#pragma once

#include <string_view>

namespace string_util{
    std::string_view trim_left_whitespace(std::string_view value);
    std::string_view trim_right_whitespace(std::string_view value);
}
