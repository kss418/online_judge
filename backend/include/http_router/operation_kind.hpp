#pragma once

#include <cstdint>
#include <string_view>

namespace http_route{
    enum class operation_kind : std::uint8_t{
        query,
        command
    };

    struct endpoint_metadata{
        std::string_view name;
        operation_kind kind;
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
