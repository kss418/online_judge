#pragma once

#include "http_endpoint/endpoint.hpp"

#include <cstdint>
#include <expected>
#include <functional>
#include <string_view>
#include <type_traits>
#include <utility>

namespace http_handler_spec{
    namespace detail{
        template <typename execute_type>
        auto make_execute_adapter(execute_type&& execute){
            using execute_storage_type = std::decay_t<execute_type>;

            return [
                execute_value = execute_storage_type(
                    std::forward<execute_type>(execute)
                )
            ](auto& context, auto&& command_value) mutable {
                if constexpr(std::is_invocable_v<
                    execute_storage_type&,
                    decltype(context.db_connection_ref()),
                    decltype(command_value)
                >){
                    return std::invoke(
                        execute_value,
                        context.db_connection_ref(),
                        std::forward<decltype(command_value)>(command_value)
                    );
                }else if constexpr(std::is_invocable_v<
                    execute_storage_type&,
                    decltype(context),
                    decltype(command_value)
                >){
                    return std::invoke(
                        execute_value,
                        context,
                        std::forward<decltype(command_value)>(command_value)
                    );
                }else{
                    static_assert(
                        std::is_invocable_v<
                            execute_storage_type&,
                            decltype(command_value)
                        >,
                        "http_handler_spec execute must accept (db_connection, command), (request_context, command), or (command)"
                    );
                    return std::invoke(
                        execute_value,
                        std::forward<decltype(command_value)>(command_value)
                    );
                }
            };
        }

        template <typename path_value_type, typename builder_type>
        auto make_path_value_builder(
            path_value_type&& path_value,
            builder_type&& build_command
        ){
            using path_value_storage_type = std::decay_t<path_value_type>;
            using builder_storage_type = std::decay_t<builder_type>;

            return [
                path_value_value = path_value_storage_type(
                    std::forward<path_value_type>(path_value)
                ),
                build_command_value = builder_storage_type(
                    std::forward<builder_type>(build_command)
                )
            ](const http_guard::guard_context& context, auto&&... guard_values) mutable {
                return std::invoke(
                    build_command_value,
                    context,
                    path_value_value,
                    std::forward<decltype(guard_values)>(guard_values)...
                );
            };
        }
    }

    template <typename serializer_factory_type>
    auto make_json_message_serializer(
        std::string_view message,
        serializer_factory_type&& serializer_factory
    ){
        using serializer_factory_storage_type =
            std::decay_t<serializer_factory_type>;

        return [
            message,
            serializer_factory_value = serializer_factory_storage_type(
                std::forward<serializer_factory_type>(serializer_factory)
            )
        ](auto&&... value_args) mutable {
            return std::invoke(
                serializer_factory_value,
                message,
                std::forward<decltype(value_args)>(value_args)...
            );
        };
    }

    template <
        typename path_value_type,
        typename builder_type,
        typename execute_type,
        typename serialize_type,
        typename... guard_types
    >
    auto make_single_path_value_json_spec(
        path_value_type&& path_value,
        builder_type&& build_command,
        execute_type&& execute,
        serialize_type&& serialize,
        guard_types&&... guards
    ){
        return http_endpoint::make_guarded_json_spec(
            detail::make_path_value_builder(
                std::forward<path_value_type>(path_value),
                std::forward<builder_type>(build_command)
            ),
            detail::make_execute_adapter(std::forward<execute_type>(execute)),
            std::forward<serialize_type>(serialize),
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
    auto make_single_path_value_json_spec(
        path_value_type&& path_value,
        builder_type&& build_command,
        execute_type&& execute,
        serialize_type&& serialize,
        http_endpoint::spec_options<error_response_factory_type> options,
        guard_types&&... guards
    ){
        return http_endpoint::make_guarded_json_spec(
            detail::make_path_value_builder(
                std::forward<path_value_type>(path_value),
                std::forward<builder_type>(build_command)
            ),
            detail::make_execute_adapter(std::forward<execute_type>(execute)),
            std::forward<serialize_type>(serialize),
            std::move(options),
            std::forward<guard_types>(guards)...
        );
    }

    template <
        typename path_value_type,
        typename execute_type,
        typename serialize_type,
        typename... guard_types
    >
    auto make_path_value_json_spec(
        path_value_type&& path_value,
        execute_type&& execute,
        serialize_type&& serialize,
        guard_types&&... guards
    ){
        using command_type = std::decay_t<path_value_type>;

        return make_single_path_value_json_spec(
            std::forward<path_value_type>(path_value),
            [](const http_guard::guard_context&, const command_type& command_value,
                auto&&...) -> std::expected<command_type, http_guard::response_type> {
                return command_value;
            },
            std::forward<execute_type>(execute),
            std::forward<serialize_type>(serialize),
            std::forward<guard_types>(guards)...
        );
    }

    template <
        typename path_value_type,
        typename builder_type,
        typename execute_type,
        typename serialize_type,
        typename... guard_types
    >
    auto make_single_path_value_message_spec(
        path_value_type&& path_value,
        builder_type&& build_command,
        execute_type&& execute,
        serialize_type&& serialize,
        guard_types&&... guards
    ){
        return http_endpoint::make_guarded_message_spec(
            detail::make_path_value_builder(
                std::forward<path_value_type>(path_value),
                std::forward<builder_type>(build_command)
            ),
            detail::make_execute_adapter(std::forward<execute_type>(execute)),
            std::forward<serialize_type>(serialize),
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
    auto make_single_path_value_message_spec(
        path_value_type&& path_value,
        builder_type&& build_command,
        execute_type&& execute,
        serialize_type&& serialize,
        http_endpoint::spec_options<error_response_factory_type> options,
        guard_types&&... guards
    ){
        return http_endpoint::make_guarded_message_spec(
            detail::make_path_value_builder(
                std::forward<path_value_type>(path_value),
                std::forward<builder_type>(build_command)
            ),
            detail::make_execute_adapter(std::forward<execute_type>(execute)),
            std::forward<serialize_type>(serialize),
            std::move(options),
            std::forward<guard_types>(guards)...
        );
    }
}
