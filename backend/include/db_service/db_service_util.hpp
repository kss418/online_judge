#pragma once

#include "common/db_connection.hpp"
#include "error/error_code.hpp"
#include "error/service_error.hpp"
#include "db_repository/db_repository.hpp"

#include <pqxx/pqxx>

#include <concepts>
#include <expected>
#include <functional>
#include <type_traits>
#include <utility>

namespace db_service_util{
    inline constexpr int DB_TRANSACTION_ATTEMPT_COUNT = 5;

    inline service_error map_error_to_service_error(const error_code& error_code_value){
        return service_error::from_error_code(error_code_value);
    }

    inline error_code map_service_error_to_error_code(service_error service_error_value){
        return error_code::create(http_error::from_service(service_error_value));
    }

    template <typename T>
    std::expected<T, service_error> map_error_to_service_error(
        std::expected<T, error_code> result_exp
    ){
        if(!result_exp){
            return std::unexpected(map_error_to_service_error(result_exp.error()));
        }

        if constexpr(std::is_void_v<T>){
            return {};
        }
        else{
            return std::expected<T, service_error>{std::move(*result_exp)};
        }
    }

    template <typename T>
    std::expected<T, error_code> map_service_error_to_error_code(
        std::expected<T, service_error> result_exp
    ){
        if(!result_exp){
            return std::unexpected(map_service_error_to_error_code(result_exp.error()));
        }

        if constexpr(std::is_void_v<T>){
            return {};
        }
        else{
            return std::expected<T, error_code>{std::move(*result_exp)};
        }
    }

    template <typename result_type>
    concept expected_error_result = requires{
        typename result_type::value_type;
        typename result_type::error_type;
    } && std::same_as<typename result_type::error_type, error_code>;

    template <typename callback_type>
    using write_transaction_result = std::invoke_result_t<callback_type, pqxx::work&>;

    template <typename callback_type>
    using read_transaction_result = std::invoke_result_t<callback_type, pqxx::read_transaction&>;

    template <typename callback_type>
        requires expected_error_result<write_transaction_result<callback_type>>
    auto with_write_transaction(
        db_connection& connection,
        callback_type&& callback
    ) -> write_transaction_result<callback_type> {
        if(!connection.is_connected()){
            return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
        }

        try{
            pqxx::work transaction(connection.connection());
            auto callback_result_exp = std::invoke(
                std::forward<callback_type>(callback),
                transaction
            );
            if(!callback_result_exp){
                return callback_result_exp;
            }

            transaction.commit();
            return callback_result_exp;
        }
        catch(const std::exception& exception){
            return std::unexpected(error_code::map_psql_error_code(exception));
        }
    }

    template <typename callback_type>
        requires expected_error_result<write_transaction_result<callback_type>>
    auto with_retry_write_transaction(
        db_connection& connection,
        int retry_count,
        callback_type&& callback
    ) -> write_transaction_result<callback_type> {
        return db_repository::retry_db_operation(
            connection,
            retry_count,
            [&]() -> write_transaction_result<callback_type> {
                return with_write_transaction(connection, callback);
            }
        );
    }

    template <typename callback_type>
        requires expected_error_result<write_transaction_result<callback_type>>
    auto with_retry_write_transaction(
        db_connection& connection,
        callback_type&& callback
    ) -> write_transaction_result<callback_type> {
        return with_retry_write_transaction(
            connection,
            DB_TRANSACTION_ATTEMPT_COUNT,
            std::forward<callback_type>(callback)
        );
    }

    template <typename callback_type>
        requires expected_error_result<read_transaction_result<callback_type>>
    auto with_read_transaction(
        db_connection& connection,
        callback_type&& callback
    ) -> read_transaction_result<callback_type> {
        if(!connection.is_connected()){
            return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
        }

        try{
            pqxx::read_transaction transaction(connection.connection());
            return std::invoke(std::forward<callback_type>(callback), transaction);
        }
        catch(const std::exception& exception){
            return std::unexpected(error_code::map_psql_error_code(exception));
        }
    }

    template <typename callback_type>
        requires expected_error_result<read_transaction_result<callback_type>>
    auto with_retry_read_transaction(
        db_connection& connection,
        int retry_count,
        callback_type&& callback
    ) -> read_transaction_result<callback_type> {
        return db_repository::retry_db_operation(
            connection,
            retry_count,
            [&]() -> read_transaction_result<callback_type> {
                return with_read_transaction(connection, callback);
            }
        );
    }

    template <typename callback_type>
        requires expected_error_result<read_transaction_result<callback_type>>
    auto with_retry_read_transaction(
        db_connection& connection,
        callback_type&& callback
    ) -> read_transaction_result<callback_type> {
        return with_retry_read_transaction(
            connection,
            DB_TRANSACTION_ATTEMPT_COUNT,
            std::forward<callback_type>(callback)
        );
    }
}
