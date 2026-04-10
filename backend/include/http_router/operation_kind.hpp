#pragma once

#include <cstdint>
#include <string_view>

namespace http_route{
    enum class operation_kind : std::uint8_t{
        query,
        command
    };

    constexpr std::string_view to_string(operation_kind operation_kind_value){
        switch(operation_kind_value){
            case operation_kind::query:
                return "query";
            case operation_kind::command:
                return "command";
        }

        return "unknown";
    }
}
