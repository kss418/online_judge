#pragma once

#include "common/db_connection.hpp"
#include "common/error_code.hpp"

#include <concepts>
#include <expected>
#include <functional>
#include <type_traits>
#include <utility>

namespace db_repository{
    bool should_retry_db_error(const error_code& error_code_value);
    bool should_reconnect_db_error(const error_code& error_code_value);
    repository_error invalid_reference_error();
    repository_error invalid_input_error();
    repository_error not_found_error();
    repository_error conflict_error();
    repository_error internal_error();
    repository_error map_error(const error_code& error_code_value);

    template <typename T>
    std::expected<T, repository_error> map_error(std::expected<T, error_code> value_exp){
        if(!value_exp){
            return std::unexpected(map_error(value_exp.error()));
        }

        if constexpr(std::is_void_v<T>){
            return {};
        }
        else{
            return std::expected<T, repository_error>{std::move(*value_exp)};
        }
    }

    template <typename result_type>
    concept expected_error_result = requires{
        typename result_type::value_type;
        typename result_type::error_type;
    } && std::same_as<typename result_type::error_type, error_code>;

    template <typename callback_type>
    using operation_result = std::invoke_result_t<callback_type>;

    template <typename callback_type>
        requires expected_error_result<operation_result<callback_type>>
    auto retry_db_operation(
        db_connection& db_connection_value,
        int retry_count,
        callback_type&& callback
    ) -> operation_result<callback_type> {
        if(retry_count <= 0){
            return std::unexpected(error_code::create(errno_error::invalid_argument));
        }

        error_code last_error = error_code::create(psql_error::unknown_psql_error);

        for(int attempt = 1; attempt <= retry_count; ++attempt){
            auto callback_result_exp = std::invoke(callback);
            if(callback_result_exp){
                return callback_result_exp;
            }

            const error_code callback_error = callback_result_exp.error();
            last_error = callback_error;
            if(
                attempt == retry_count ||
                !should_retry_db_error(callback_error)
            ){
                return std::unexpected(callback_error);
            }

            if(should_reconnect_db_error(callback_error)){
                const auto reconnect_exp = db_connection_value.reconnect();
                if(!reconnect_exp){
                    last_error = reconnect_exp.error();
                }
            }
        }

        return std::unexpected(last_error);
    }
}
