#pragma once

#include <string_view>

struct query_param{
    std::string_view key;
    std::string_view value;
};
