#pragma once

#include "common/db_connection.hpp"
#include "http_core/http_response_util.hpp"

#include <expected>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>

namespace http_guard{
    using request_type = http_response_util::request_type;
    using response_type = http_response_util::response_type;

    struct guard_context{
        const request_type& request;
        db_connection& db_connection_value;
    };

    namespace detail{
        template <typename type>
        struct is_expected : std::false_type{};

        template <typename value_type, typename error_type>
        struct is_expected<std::expected<value_type, error_type>> : std::true_type{};

        template <typename expected_type>
        struct expected_traits;

        template <typename value_type, typename error_type>
        struct expected_traits<std::expected<value_type, error_type>>{
            using value = value_type;
            using error = error_type;
        };

        template <typename guard_type>
        using guard_result_type = std::remove_cvref_t<
            std::invoke_result_t<guard_type, const guard_context&>
        >;

        template <typename guard_type>
        using guard_value_type = typename expected_traits<
            guard_result_type<guard_type>
        >::value;

        template <typename guard_type>
        using guard_error_type = typename expected_traits<
            guard_result_type<guard_type>
        >::error;

        template <typename guard_type>
        using guard_tuple_type = std::conditional_t<
            std::is_void_v<guard_value_type<guard_type>>,
            std::tuple<>,
            std::tuple<guard_value_type<guard_type>>
        >;

        template <typename... guard_types>
        struct combined_guard_tuple{
            using type = std::tuple<>;
        };

        template <typename guard_type, typename... remaining_guard_types>
        struct combined_guard_tuple<guard_type, remaining_guard_types...>{
            using type = decltype(std::tuple_cat(
                std::declval<guard_tuple_type<guard_type>>(),
                std::declval<typename combined_guard_tuple<remaining_guard_types...>::type>()
            ));
        };

        template <typename... guard_types>
        using combined_guard_tuple_type = typename combined_guard_tuple<guard_types...>::type;

        template <typename guard_type>
        guard_tuple_type<guard_type> make_guard_tuple(
            guard_result_type<guard_type>&& guard_result
        ){
            if constexpr(std::is_void_v<guard_value_type<guard_type>>){
                return {};
            }else{
                return std::tuple<guard_value_type<guard_type>>{
                    std::move(*guard_result)
                };
            }
        }
    }

    inline std::expected<std::tuple<>, response_type> run(const guard_context&){
        return std::tuple<>{};
    }

    template <typename guard_type, typename... remaining_guard_types>
    std::expected<
        detail::combined_guard_tuple_type<guard_type, remaining_guard_types...>,
        response_type
    > run(
        const guard_context& context,
        guard_type&& guard,
        remaining_guard_types&&... remaining_guards
    ){
        using guard_result_type = detail::guard_result_type<guard_type>;

        static_assert(
            detail::is_expected<guard_result_type>::value,
            "http_guard::run expects every guard to return std::expected"
        );
        static_assert(
            std::is_same_v<detail::guard_error_type<guard_type>, response_type>,
            "http_guard::run expects every guard to use http response_type as the error type"
        );

        auto guard_result = std::invoke(
            std::forward<guard_type>(guard),
            context
        );
        if(!guard_result){
            return std::unexpected(std::move(guard_result.error()));
        }

        auto remaining_result = run(
            context,
            std::forward<remaining_guard_types>(remaining_guards)...
        );
        if(!remaining_result){
            return std::unexpected(std::move(remaining_result.error()));
        }

        return std::tuple_cat(
            detail::make_guard_tuple<guard_type>(std::move(guard_result)),
            std::move(*remaining_result)
        );
    }

    template <typename success_type, typename... guard_types>
    response_type run_or_respond(
        const guard_context& context,
        success_type&& success,
        guard_types&&... guards
    ){
        auto guard_values_exp = run(
            context,
            std::forward<guard_types>(guards)...
        );
        if(!guard_values_exp){
            return std::move(guard_values_exp.error());
        }

        return std::apply(
            [&](auto&&... values) -> response_type {
                return std::invoke(
                    std::forward<success_type>(success),
                    std::forward<decltype(values)>(values)...
                );
            },
            std::move(*guard_values_exp)
        );
    }
}
