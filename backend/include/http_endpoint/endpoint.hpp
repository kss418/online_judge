#pragma once

#include "http_core/http_adapter.hpp"
#include "http_guard/guard_runner.hpp"

#include <boost/beast/http/status.hpp>

#include <expected>
#include <functional>
#include <type_traits>
#include <utility>

namespace http_endpoint{
    struct no_input{};

    struct identity_serializer{
        template <typename value_type>
        decltype(auto) operator()(value_type&& value) const{
            return std::forward<value_type>(value);
        }
    };

    struct default_error_response_factory{
        template <typename error_type>
        http_adapter::response_type operator()(
            const http_adapter::request_type& request,
            const error_type& error_value
        ) const{
            return http_response_util::create_4xx_or_500(
                request,
                error_value
            );
        }
    };

    template <typename error_response_factory_type = default_error_response_factory>
    struct spec_options{
        error_response_factory_type error_response{};
        boost::beast::http::status success_status =
            boost::beast::http::status::ok;
    };

    template <
        typename parse_type,
        typename execute_type,
        typename serialize_type = identity_serializer,
        typename error_response_factory_type = default_error_response_factory
    >
    struct endpoint_spec{
        parse_type parse;
        execute_type execute;
        serialize_type serialize;
        error_response_factory_type error_response;
        boost::beast::http::status success_status =
            boost::beast::http::status::ok;
    };

    inline auto make_no_input_guard(){
        return [](const http_guard::guard_context&)
            -> std::expected<no_input, http_guard::response_type> {
            return no_input{};
        };
    }

    template <typename execute_type>
    auto make_db_execute(execute_type&& execute){
        using execute_storage_type = std::decay_t<execute_type>;

        return [
            execute_value = execute_storage_type(
                std::forward<execute_type>(execute)
            )
        ](auto& context, auto&& command_value) {
            return std::invoke(
                execute_value,
                context.db_connection_ref(),
                std::forward<decltype(command_value)>(command_value)
            );
        };
    }

    namespace detail{
        template <typename context_type, typename spec_type, typename value_type, typename error_type>
        auto create_json_response(
            context_type& context,
            spec_type& spec_value,
            std::expected<value_type, error_type> result
        ) -> typename context_type::response_type{
            return http_adapter::json(
                context.request,
                std::move(result),
                [&spec_value](const auto& request_value, const auto& error_value) {
                    return std::invoke(
                        spec_value.error_response,
                        request_value,
                        error_value
                    );
                },
                [&spec_value](auto&&... value_args) {
                    return std::invoke(
                        spec_value.serialize,
                        std::forward<decltype(value_args)>(value_args)...
                    );
                },
                spec_value.success_status
            );
        }

        template <typename context_type, typename spec_type, typename value_type>
        auto create_json_response(
            context_type& context,
            spec_type& spec_value,
            value_type&& result
        ) -> typename context_type::response_type{
            return http_adapter::json(
                context.request,
                std::invoke(
                    spec_value.serialize,
                    std::forward<value_type>(result)
                ),
                spec_value.success_status
            );
        }

        template <typename context_type, typename spec_type, typename value_type, typename error_type>
        auto create_message_response(
            context_type& context,
            spec_type& spec_value,
            std::expected<value_type, error_type> result
        ) -> typename context_type::response_type{
            return http_adapter::response(
                context.request,
                std::move(result),
                [&spec_value](const auto& request_value, const auto& error_value) {
                    return std::invoke(
                        spec_value.error_response,
                        request_value,
                        error_value
                    );
                },
                [&context, &spec_value](auto&&... value_args) {
                    return http_response_util::create_message(
                        context.request,
                        spec_value.success_status,
                        std::invoke(
                            spec_value.serialize,
                            std::forward<decltype(value_args)>(value_args)...
                        )
                    );
                }
            );
        }

        template <typename context_type, typename spec_type, typename value_type>
        auto create_message_response(
            context_type& context,
            spec_type& spec_value,
            value_type&& result
        ) -> typename context_type::response_type{
            return http_response_util::create_message(
                context.request,
                spec_value.success_status,
                std::invoke(
                    spec_value.serialize,
                    std::forward<value_type>(result)
                )
            );
        }
    }

    template <typename context_type, typename spec_type>
    auto run_json(context_type& context, spec_type&& spec) -> typename context_type::response_type{
        auto spec_value = std::forward<spec_type>(spec);
        return http_guard::run_or_respond(
            context,
            [&context, &spec_value](auto&& parsed_value) -> typename context_type::response_type {
                auto result = std::invoke(
                    spec_value.execute,
                    context,
                    std::forward<decltype(parsed_value)>(parsed_value)
                );
                return detail::create_json_response(
                    context,
                    spec_value,
                    std::move(result)
                );
            },
            std::move(spec_value.parse)
        );
    }

    template <typename context_type, typename spec_type>
    auto run_message(context_type& context, spec_type&& spec)
        -> typename context_type::response_type{
        auto spec_value = std::forward<spec_type>(spec);
        return http_guard::run_or_respond(
            context,
            [&context, &spec_value](auto&& parsed_value) -> typename context_type::response_type {
                auto result = std::invoke(
                    spec_value.execute,
                    context,
                    std::forward<decltype(parsed_value)>(parsed_value)
                );
                return detail::create_message_response(
                    context,
                    spec_value,
                    std::move(result)
                );
            },
            std::move(spec_value.parse)
        );
    }

    template <
        typename parse_type,
        typename execute_type,
        typename serialize_type
    >
    auto make_json_spec(
        parse_type&& parse,
        execute_type&& execute,
        serialize_type&& serialize
    ){
        return endpoint_spec{
            .parse = std::decay_t<parse_type>(std::forward<parse_type>(parse)),
            .execute = std::decay_t<execute_type>(std::forward<execute_type>(execute)),
            .serialize = std::decay_t<serialize_type>(std::forward<serialize_type>(serialize)),
            .error_response = default_error_response_factory{},
            .success_status = boost::beast::http::status::ok
        };
    }

    template <
        typename parse_type,
        typename execute_type,
        typename serialize_type,
        typename error_response_factory_type
    >
    auto make_json_spec(
        parse_type&& parse,
        execute_type&& execute,
        serialize_type&& serialize,
        spec_options<error_response_factory_type> options
    ){
        return endpoint_spec{
            .parse = std::decay_t<parse_type>(std::forward<parse_type>(parse)),
            .execute = std::decay_t<execute_type>(std::forward<execute_type>(execute)),
            .serialize = std::decay_t<serialize_type>(std::forward<serialize_type>(serialize)),
            .error_response = std::move(options.error_response),
            .success_status = options.success_status
        };
    }

    template <
        typename builder_type,
        typename execute_type,
        typename serialize_type,
        typename... guard_types
    >
    auto make_guarded_json_spec(
        builder_type&& build_command,
        execute_type&& execute,
        serialize_type&& serialize,
        guard_types&&... guards
    ){
        return make_json_spec(
            http_guard::make_composite_guard(
                std::forward<builder_type>(build_command),
                std::forward<guard_types>(guards)...
            ),
            std::forward<execute_type>(execute),
            std::forward<serialize_type>(serialize)
        );
    }

    template <
        typename builder_type,
        typename execute_type,
        typename serialize_type,
        typename error_response_factory_type,
        typename... guard_types
    >
    auto make_guarded_json_spec(
        builder_type&& build_command,
        execute_type&& execute,
        serialize_type&& serialize,
        spec_options<error_response_factory_type> options,
        guard_types&&... guards
    ){
        return make_json_spec(
            http_guard::make_composite_guard(
                std::forward<builder_type>(build_command),
                std::forward<guard_types>(guards)...
            ),
            std::forward<execute_type>(execute),
            std::forward<serialize_type>(serialize),
            std::move(options)
        );
    }

    template <
        typename parse_type,
        typename execute_type,
        typename serialize_type
    >
    auto make_message_spec(
        parse_type&& parse,
        execute_type&& execute,
        serialize_type&& serialize
    ){
        return endpoint_spec{
            .parse = std::decay_t<parse_type>(std::forward<parse_type>(parse)),
            .execute = std::decay_t<execute_type>(std::forward<execute_type>(execute)),
            .serialize = std::decay_t<serialize_type>(std::forward<serialize_type>(serialize)),
            .error_response = default_error_response_factory{},
            .success_status = boost::beast::http::status::ok
        };
    }

    template <
        typename parse_type,
        typename execute_type,
        typename serialize_type,
        typename error_response_factory_type
    >
    auto make_message_spec(
        parse_type&& parse,
        execute_type&& execute,
        serialize_type&& serialize,
        spec_options<error_response_factory_type> options
    ){
        return endpoint_spec{
            .parse = std::decay_t<parse_type>(std::forward<parse_type>(parse)),
            .execute = std::decay_t<execute_type>(std::forward<execute_type>(execute)),
            .serialize = std::decay_t<serialize_type>(std::forward<serialize_type>(serialize)),
            .error_response = std::move(options.error_response),
            .success_status = options.success_status
        };
    }

    template <
        typename builder_type,
        typename execute_type,
        typename serialize_type,
        typename... guard_types
    >
    auto make_guarded_message_spec(
        builder_type&& build_command,
        execute_type&& execute,
        serialize_type&& serialize,
        guard_types&&... guards
    ){
        return make_message_spec(
            http_guard::make_composite_guard(
                std::forward<builder_type>(build_command),
                std::forward<guard_types>(guards)...
            ),
            std::forward<execute_type>(execute),
            std::forward<serialize_type>(serialize)
        );
    }

    template <
        typename builder_type,
        typename execute_type,
        typename serialize_type,
        typename error_response_factory_type,
        typename... guard_types
    >
    auto make_guarded_message_spec(
        builder_type&& build_command,
        execute_type&& execute,
        serialize_type&& serialize,
        spec_options<error_response_factory_type> options,
        guard_types&&... guards
    ){
        return make_message_spec(
            http_guard::make_composite_guard(
                std::forward<builder_type>(build_command),
                std::forward<guard_types>(guards)...
            ),
            std::forward<execute_type>(execute),
            std::forward<serialize_type>(serialize),
            std::move(options)
        );
    }
}
