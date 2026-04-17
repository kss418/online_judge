#pragma once

#include "http_handler/path_value_spec_helper.hpp"
#include "http_guard/user_guard.hpp"

#include <cstdint>
#include <expected>
#include <utility>

namespace http_handler_spec{
    template <
        typename execute_type,
        typename serialize_type
    >
    auto make_existing_user_json_spec(
        std::int64_t user_id,
        execute_type&& execute,
        serialize_type&& serialize
    ){
        return make_path_value_json_spec(
            user_id,
            std::forward<execute_type>(execute),
            std::forward<serialize_type>(serialize),
            user_guard::make_exists_guard(user_id)
        );
    }

    template <
        typename execute_type,
        typename serialize_type,
        typename error_response_factory_type
    >
    auto make_existing_user_json_spec(
        std::int64_t user_id,
        execute_type&& execute,
        serialize_type&& serialize,
        http_endpoint::spec_options<error_response_factory_type> options
    ){
        return make_single_path_value_json_spec(
            user_id,
            [](const http_guard::guard_context&, std::int64_t user_id_value)
                -> std::expected<std::int64_t, http_guard::response_type> {
                return user_id_value;
            },
            std::forward<execute_type>(execute),
            std::forward<serialize_type>(serialize),
            std::move(options),
            user_guard::make_exists_guard(user_id)
        );
    }
}
