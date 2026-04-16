#pragma once

#include "http_handler/path_value_spec_helper.hpp"
#include "http_guard/auth_guard.hpp"
#include "http_guard/user_guard.hpp"

#include <cstdint>
#include <utility>

namespace http_handler_spec{
    template <
        typename builder_type,
        typename execute_type,
        typename serialize_type,
        typename... guard_types
    >
    auto make_admin_user_json_spec(
        std::int64_t user_id,
        builder_type&& build_command,
        execute_type&& execute,
        serialize_type&& serialize,
        guard_types&&... guards
    ){
        return make_single_path_value_json_spec(
            user_id,
            std::forward<builder_type>(build_command),
            std::forward<execute_type>(execute),
            std::forward<serialize_type>(serialize),
            auth_guard::make_admin_guard(),
            user_guard::make_exists_guard(user_id),
            std::forward<guard_types>(guards)...
        );
    }

    template <
        typename builder_type,
        typename execute_type,
        typename serialize_type,
        typename error_response_factory_type,
        typename... guard_types
    >
    auto make_admin_user_json_spec(
        std::int64_t user_id,
        builder_type&& build_command,
        execute_type&& execute,
        serialize_type&& serialize,
        http_endpoint::spec_options<error_response_factory_type> options,
        guard_types&&... guards
    ){
        return make_single_path_value_json_spec(
            user_id,
            std::forward<builder_type>(build_command),
            std::forward<execute_type>(execute),
            std::forward<serialize_type>(serialize),
            std::move(options),
            auth_guard::make_admin_guard(),
            user_guard::make_exists_guard(user_id),
            std::forward<guard_types>(guards)...
        );
    }

    template <
        typename builder_type,
        typename execute_type,
        typename serialize_type,
        typename... guard_types
    >
    auto make_admin_user_message_spec(
        std::int64_t user_id,
        builder_type&& build_command,
        execute_type&& execute,
        serialize_type&& serialize,
        guard_types&&... guards
    ){
        return make_single_path_value_message_spec(
            user_id,
            std::forward<builder_type>(build_command),
            std::forward<execute_type>(execute),
            std::forward<serialize_type>(serialize),
            auth_guard::make_admin_guard(),
            user_guard::make_exists_guard(user_id),
            std::forward<guard_types>(guards)...
        );
    }

    template <
        typename builder_type,
        typename execute_type,
        typename serialize_type,
        typename error_response_factory_type,
        typename... guard_types
    >
    auto make_admin_user_message_spec(
        std::int64_t user_id,
        builder_type&& build_command,
        execute_type&& execute,
        serialize_type&& serialize,
        http_endpoint::spec_options<error_response_factory_type> options,
        guard_types&&... guards
    ){
        return make_single_path_value_message_spec(
            user_id,
            std::forward<builder_type>(build_command),
            std::forward<execute_type>(execute),
            std::forward<serialize_type>(serialize),
            std::move(options),
            auth_guard::make_admin_guard(),
            user_guard::make_exists_guard(user_id),
            std::forward<guard_types>(guards)...
        );
    }
}
