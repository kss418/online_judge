#pragma once

#include "http_handler/path_value_spec_helper.hpp"
#include "http_guard/auth_guard.hpp"

#include <utility>

namespace http_handler_spec{
    template <
        typename builder_type,
        typename execute_type,
        typename serialize_type
    >
    auto make_auth_identity_json_spec(
        builder_type&& build_command,
        execute_type&& execute,
        serialize_type&& serialize
    ){
        return http_endpoint::make_guarded_json_spec(
            std::forward<builder_type>(build_command),
            detail::make_execute_adapter(std::forward<execute_type>(execute)),
            std::forward<serialize_type>(serialize),
            auth_guard::make_auth_guard()
        );
    }

    template <
        typename builder_type,
        typename execute_type,
        typename serialize_type,
        typename error_response_factory_type
    >
    auto make_auth_identity_json_spec(
        builder_type&& build_command,
        execute_type&& execute,
        serialize_type&& serialize,
        http_endpoint::spec_options<error_response_factory_type> options
    ){
        return http_endpoint::make_guarded_json_spec(
            std::forward<builder_type>(build_command),
            detail::make_execute_adapter(std::forward<execute_type>(execute)),
            std::forward<serialize_type>(serialize),
            std::move(options),
            auth_guard::make_auth_guard()
        );
    }

    template <
        typename path_value_type,
        typename builder_type,
        typename execute_type,
        typename serialize_type,
        typename... guard_types
    >
    auto make_auth_optional_path_json_spec(
        path_value_type&& path_value,
        builder_type&& build_command,
        execute_type&& execute,
        serialize_type&& serialize,
        guard_types&&... guards
    ){
        return make_single_path_value_json_spec(
            std::forward<path_value_type>(path_value),
            std::forward<builder_type>(build_command),
            std::forward<execute_type>(execute),
            std::forward<serialize_type>(serialize),
            auth_guard::make_optional_auth_guard(),
            std::forward<guard_types>(guards)...
        );
    }

    template <
        typename path_value_type,
        typename builder_type,
        typename execute_type,
        typename serialize_type,
        typename error_response_factory_type,
        typename... guard_types
    >
    auto make_auth_optional_path_json_spec(
        path_value_type&& path_value,
        builder_type&& build_command,
        execute_type&& execute,
        serialize_type&& serialize,
        http_endpoint::spec_options<error_response_factory_type> options,
        guard_types&&... guards
    ){
        return make_single_path_value_json_spec(
            std::forward<path_value_type>(path_value),
            std::forward<builder_type>(build_command),
            std::forward<execute_type>(execute),
            std::forward<serialize_type>(serialize),
            std::move(options),
            auth_guard::make_optional_auth_guard(),
            std::forward<guard_types>(guards)...
        );
    }
}
