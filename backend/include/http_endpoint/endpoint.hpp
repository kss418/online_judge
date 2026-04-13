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
}
