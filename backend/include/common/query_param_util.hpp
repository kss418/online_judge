#pragma once

#include "common/query_param.hpp"
#include "error/http_error.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <expected>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace query_param_util{
    template <typename filter_type>
    struct query_param_binding{
        std::string_view key;
        std::expected<void, http_error> (*apply)(
            filter_type& filter_value,
            std::string_view key,
            std::string_view raw_value
        );
    };

    template <
        typename value_type,
        typename parse_fn,
        typename duplicate_error_fn,
        typename invalid_error_fn
    >
    std::expected<void, http_error> parse_unique_query_param(
        std::optional<value_type>& out,
        std::string_view key,
        std::string_view raw_value,
        parse_fn&& parse,
        duplicate_error_fn&& make_duplicate_error,
        invalid_error_fn&& make_invalid_error
    ){
        if(out){
            return std::unexpected(
                std::invoke(std::forward<duplicate_error_fn>(make_duplicate_error), key)
            );
        }

        auto parsed_value_opt = std::invoke(
            std::forward<parse_fn>(parse),
            raw_value
        );
        if(!parsed_value_opt){
            return std::unexpected(
                std::invoke(std::forward<invalid_error_fn>(make_invalid_error), key)
            );
        }

        out = std::move(*parsed_value_opt);
        return {};
    }

    template <typename filter_type, std::size_t binding_count, typename unsupported_error_fn>
    std::expected<filter_type, http_error> make_filter_from_query_params(
        const std::vector<query_param>& query_params,
        const std::array<query_param_binding<filter_type>, binding_count>& bindings,
        unsupported_error_fn&& make_unsupported_error
    ){
        filter_type filter_value;

        for(const auto& current_query_param : query_params){
            const auto binding_it = std::find_if(
                bindings.begin(),
                bindings.end(),
                [&](const auto& binding){
                    return binding.key == current_query_param.key;
                }
            );
            if(binding_it == bindings.end()){
                return std::unexpected(
                    std::invoke(
                        std::forward<unsupported_error_fn>(make_unsupported_error),
                        current_query_param.key
                    )
                );
            }

            const auto apply_exp = binding_it->apply(
                filter_value,
                current_query_param.key,
                current_query_param.value
            );
            if(!apply_exp){
                return std::unexpected(apply_exp.error());
            }
        }

        return filter_value;
    }
}
