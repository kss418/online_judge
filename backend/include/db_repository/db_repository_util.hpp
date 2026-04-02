#pragma once

#include "error/db_error.hpp"
#include "error/error_code.hpp"
#include "error/repository_error.hpp"

#include <expected>
#include <type_traits>
#include <utility>

namespace db_repository_util{
    template <typename T>
    std::expected<T, repository_error> map_exp(
        std::expected<T, db_error> value_exp
    ){
        if(!value_exp){
            return std::unexpected(
                repository_error::from_db_error(value_exp.error())
            );
        }

        if constexpr(std::is_void_v<T>){
            return {};
        }
        else{
            return std::expected<T, repository_error>{std::move(*value_exp)};
        }
    }

    template <typename T>
    std::expected<T, repository_error> map_exp(
        std::expected<T, error_code> value_exp
    ){
        if(!value_exp){
            return std::unexpected(
                repository_error::from_error_code(value_exp.error())
            );
        }

        if constexpr(std::is_void_v<T>){
            return {};
        }
        else{
            return std::expected<T, repository_error>{std::move(*value_exp)};
        }
    }
}
