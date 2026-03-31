#pragma once

#include "dto/dto_validation_error.hpp"

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

namespace http_util{
    struct query_param{
        std::string_view key;
        std::string_view value;
    };
}

namespace query_param_util{
    dto_validation_error make_duplicate_query_parameter_error(
        std::string_view key
    );
    dto_validation_error make_invalid_query_parameter_error(
        std::string_view key
    );
    dto_validation_error make_unsupported_query_parameter_error(
        std::string_view key
    );

    template <typename filter_type>
    struct query_param_binding{
        std::string_view key;
        std::expected<void, dto_validation_error> (*apply)(
            filter_type& filter_value,
            std::string_view key,
            std::string_view raw_value
        );
    };

    template <typename value_type, typename parse_fn>
    std::expected<void, dto_validation_error> parse_unique_query_param(
        std::optional<value_type>& out,
        std::string_view key,
        std::string_view raw_value,
        parse_fn&& parse
    ){
        if(out){
            return std::unexpected(make_duplicate_query_parameter_error(key));
        }

        auto parsed_value_opt = std::invoke(
            std::forward<parse_fn>(parse),
            raw_value
        );
        if(!parsed_value_opt){
            return std::unexpected(make_invalid_query_parameter_error(key));
        }

        out = std::move(*parsed_value_opt);
        return {};
    }

    template <typename filter_type, std::size_t binding_count>
    std::expected<filter_type, dto_validation_error> make_filter_from_query_params(
        const std::vector<http_util::query_param>& query_params,
        const std::array<query_param_binding<filter_type>, binding_count>& bindings
    ){
        filter_type filter_value;

        for(const auto& query_param : query_params){
            const auto binding_it = std::find_if(
                bindings.begin(),
                bindings.end(),
                [&](const auto& binding){
                    return binding.key == query_param.key;
                }
            );
            if(binding_it == bindings.end()){
                return std::unexpected(
                    make_unsupported_query_parameter_error(query_param.key)
                );
            }

            const auto apply_exp = binding_it->apply(
                filter_value,
                query_param.key,
                query_param.value
            );
            if(!apply_exp){
                return std::unexpected(apply_exp.error());
            }
        }

        return filter_value;
    }
}
