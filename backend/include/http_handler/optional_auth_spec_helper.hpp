#pragma once

#include "http_handler/path_value_spec_helper.hpp"
#include "http_guard/auth_guard.hpp"
#include "http_guard/user_guard.hpp"

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
        typename execute_type,
        typename serialize_type
    >
    auto make_auth_user_id_json_spec(
        execute_type&& execute,
        serialize_type&& serialize
    ){
        return make_auth_identity_json_spec(
            [](const http_guard::guard_context&,
                const auth_dto::identity& auth_identity_value)
                -> std::expected<std::int64_t, http_guard::response_type> {
                return auth_identity_value.user_id;
            },
            std::forward<execute_type>(execute),
            std::forward<serialize_type>(serialize)
        );
    }

    template <
        typename execute_type,
        typename serialize_type,
        typename error_response_factory_type
    >
    auto make_auth_user_id_json_spec(
        execute_type&& execute,
        serialize_type&& serialize,
        http_endpoint::spec_options<error_response_factory_type> options
    ){
        return make_auth_identity_json_spec(
            [](const http_guard::guard_context&,
                const auth_dto::identity& auth_identity_value)
                -> std::expected<std::int64_t, http_guard::response_type> {
                return auth_identity_value.user_id;
            },
            std::forward<execute_type>(execute),
            std::forward<serialize_type>(serialize),
            std::move(options)
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

    template <
        typename builder_type,
        typename execute_type,
        typename serialize_type,
        typename... guard_types
    >
    auto make_auth_optional_existing_user_json_spec(
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
            auth_guard::make_optional_auth_guard(),
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
    auto make_auth_optional_existing_user_json_spec(
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
            auth_guard::make_optional_auth_guard(),
            user_guard::make_exists_guard(user_id),
            std::forward<guard_types>(guards)...
        );
    }
}
