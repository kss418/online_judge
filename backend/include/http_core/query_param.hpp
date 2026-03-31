#pragma once

#include <string_view>

namespace request_parser{
    struct query_param{
        std::string_view key;
        std::string_view value;
    };
}
