#pragma once

#include "dto/submission_dto.hpp"
#include "http_guard/auth_guard.hpp"
#include "http_core/request_dto.hpp"
#include "http_core/request_list_filter_dto.hpp"
#include "http_guard/guard_runner.hpp"
#include "http_guard/problem_guard.hpp"
#include "request_parser/submission_request_parser.hpp"

#include <cstdint>
#include <tuple>
#include <type_traits>
#include <utility>

namespace request_guard{
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
                        context.request,
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
                        context.request,
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
            return request_dto::parse_problem_list_filter_or_400(context.request);
        };
    }

    inline auto make_submission_list_filter_guard(){
        return [](const http_guard::guard_context& context){
            return request_dto::parse_submission_list_filter_or_400(context.request);
        };
    }

    inline auto make_submission_create_request_guard(std::int64_t problem_id){
        return http_guard::make_composite_guard(
            [problem_id](const http_guard::guard_context& composite_context,
                const auth_dto::identity& auth_identity_value)
                -> std::expected<submission_dto::create_request, http_guard::response_type> {
                auto create_request_exp =
                    request_dto::parse_json_or_400<submission_dto::create_request>(
                    composite_context.request,
                    submission_request_parser::parse_create_request,
                    auth_identity_value.user_id,
                    problem_id
                );
                if(!create_request_exp){
                    return std::unexpected(std::move(create_request_exp.error()));
                }

                auto problem_exists_exp = problem_guard::require_exists(
                    composite_context.request_context_value,
                    problem_dto::reference{problem_id}
                );
                if(!problem_exists_exp){
                    return std::unexpected(std::move(problem_exists_exp.error()));
                }

                return create_request_exp;
            },
            auth_guard::make_auth_guard()
        );
    }
}
