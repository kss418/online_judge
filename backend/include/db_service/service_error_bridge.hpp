#pragma once

#include "db_service/db_service_util.hpp"
#include "error/service_error.hpp"

#include <expected>
#include <type_traits>
#include <utility>

namespace db_service_error_bridge{
    inline error_code map_repository_error(repository_error repository_error_value){
        return db_service_util::map_service_error_to_error_code(
            service_error::from_repository(repository_error_value)
        );
    }

    template <typename T>
    std::expected<T, error_code> map_repository_error(
        std::expected<T, repository_error> result_exp
    ){
        if(!result_exp){
            return std::unexpected(map_repository_error(result_exp.error()));
        }

        if constexpr(std::is_void_v<T>){
            return {};
        }
        else{
            return std::expected<T, error_code>{std::move(*result_exp)};
        }
    }
}
