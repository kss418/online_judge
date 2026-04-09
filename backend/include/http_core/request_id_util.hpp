#pragma once

#include "http_core/http_response_util.hpp"

#include <atomic>
#include <cstdint>
#include <string>
#include <string_view>

namespace request_id_util{
    inline constexpr std::string_view response_header_name = "X-Request-Id";

    inline std::string make_request_id(){
        static std::atomic<std::uint64_t> next_request_id{1};
        return "req-" + std::to_string(
            next_request_id.fetch_add(1, std::memory_order_relaxed)
        );
    }

    inline void set_response_header(
        http_response_util::response_type& response,
        std::string_view request_id
    ){
        if(request_id.empty()){
            return;
        }

        response.set(response_header_name, request_id);
    }
}
