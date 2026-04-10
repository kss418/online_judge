#pragma once

#include "http_core/request_dto.hpp"
#include "http_core/request_list_filter_dto.hpp"
#include "http_guard/guard_runner.hpp"

#include <tuple>
#include <type_traits>
#include <utility>

namespace request_parse_guard{
    template <typename dto_type, typename factory_type, typename... arg_types>
    auto make_json_guard(factory_type&& factory, arg_types&&... args){
        using factory_storage_type = std::decay_t<factory_type>;
        using arg_tuple_type = std::tuple<std::decay_t<arg_types>...>;

        return [
            factory_value = factory_storage_type(std::forward<factory_type>(factory)),
            arg_values = arg_tuple_type(std::forward<arg_types>(args)...)
        ](const http_guard::guard_context& context) mutable {
            return std::apply(
                [&](auto&... captured_args){
                    return request_dto::parse_json_or_400<dto_type>(
                        context.request(),
                        factory_value,
                        captured_args...
                    );
                },
                arg_values
            );
        };
    }

    template <typename dto_type, typename factory_type, typename... arg_types>
    auto make_query_guard(factory_type&& factory, arg_types&&... args){
        using factory_storage_type = std::decay_t<factory_type>;
        using arg_tuple_type = std::tuple<std::decay_t<arg_types>...>;

        return [
            factory_value = factory_storage_type(std::forward<factory_type>(factory)),
            arg_values = arg_tuple_type(std::forward<arg_types>(args)...)
        ](const http_guard::guard_context& context) mutable {
            return std::apply(
                [&](auto&... captured_args){
                    return request_dto::parse_query_or_400<dto_type>(
                        context.request(),
                        factory_value,
                        captured_args...
                    );
                },
                arg_values
            );
        };
    }

    inline auto make_problem_list_filter_guard(){
        return [](const http_guard::guard_context& context){
            return request_dto::parse_problem_list_filter_or_400(context.request());
        };
    }

    inline auto make_submission_list_filter_guard(){
        return [](const http_guard::guard_context& context){
            return request_dto::parse_submission_list_filter_or_400(context.request());
        };
    }
}
