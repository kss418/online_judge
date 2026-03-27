#pragma once

#include "dto/dto_validation_error.hpp"

#include <expected>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

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
}
