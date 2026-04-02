#pragma once

#include "common/db_connection.hpp"
#include "error/db_error.hpp"
#include "error/error_code.hpp"
#include "error/repository_error.hpp"
#include "error/service_error.hpp"

#include <pqxx/pqxx>

#include <concepts>
#include <expected>
#include <functional>
#include <type_traits>
#include <utility>

namespace db_service_util{
    inline constexpr int DB_TRANSACTION_ATTEMPT_COUNT = 5;

    inline service_error map_db_error_to_service_error(const db_error& error_code_value){
        return service_error::from_db_error(error_code_value);
    }

    inline service_error map_error_to_service_error(const error_code& error_code_value){
        return service_error::from_error_code(error_code_value);
    }

    inline service_error map_repository_error_to_service_error(
        const repository_error& repository_error_value
    ){
        return service_error::from_repository(repository_error_value);
    }

    template <typename T>
    std::expected<T, service_error> map_db_error_to_service_error(
        std::expected<T, db_error> result_exp
    ){
        if(!result_exp){
            return std::unexpected(map_db_error_to_service_error(result_exp.error()));
        }

        if constexpr(std::is_void_v<T>){
            return {};
        }
        else{
            return std::expected<T, service_error>{std::move(*result_exp)};
        }
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
    std::expected<T, service_error> map_repository_error_to_service_error(
        std::expected<T, repository_error> result_exp
    ){
        if(!result_exp){
            return std::unexpected(
                map_repository_error_to_service_error(result_exp.error())
            );
        }

        if constexpr(std::is_void_v<T>){
            return {};
        }
        else{
            return std::expected<T, service_error>{std::move(*result_exp)};
        }
    }

    template <typename result_type>
    concept expected_db_result = requires{
        typename result_type::value_type;
        typename result_type::error_type;
    } && std::same_as<typename result_type::error_type, db_error>;

    template <typename result_type>
    concept expected_service_result = requires{
        typename result_type::value_type;
        typename result_type::error_type;
    } && std::same_as<typename result_type::error_type, service_error>;

    template <typename callback_type>
    using write_transaction_result = std::invoke_result_t<callback_type, pqxx::work&>;

    template <typename callback_type>
    using read_transaction_result = std::invoke_result_t<callback_type, pqxx::read_transaction&>;

    template <typename callback_type>
    using service_write_transaction_result =
        std::invoke_result_t<callback_type, pqxx::work&>;

    template <typename callback_type>
    using service_read_transaction_result =
        std::invoke_result_t<callback_type, pqxx::read_transaction&>;

    template <typename callback_type>
    using operation_result = std::invoke_result_t<callback_type>;

    namespace detail{
        inline std::expected<void, service_error> handle_service_db_error(
            db_connection& connection,
            int attempt,
            int retry_count,
            db_error raw_error
        ){
            if(
                attempt == retry_count ||
                !repository_error::should_retry_db_error(raw_error)
            ){
                return std::unexpected(map_db_error_to_service_error(raw_error));
            }

            if(repository_error::should_reconnect_db_error(raw_error)){
                const auto reconnect_exp = connection.reconnect();
                if(!reconnect_exp){
                    const auto reconnect_error = reconnect_exp.error();
                    if(
                        attempt == retry_count ||
                        !repository_error::should_retry_db_error(reconnect_error)
                    ){
                        return std::unexpected(
                            map_db_error_to_service_error(reconnect_error)
                        );
                    }
                }
            }

            return {};
        }
    }

    template <typename callback_type>
        requires expected_db_result<operation_result<callback_type>>
    auto retry_db_operation(
        db_connection& db_connection_value,
        int retry_count,
        callback_type&& callback
    ) -> operation_result<callback_type> {
        if(retry_count <= 0){
            return std::unexpected(db_error::invalid_argument);
        }

        db_error last_error = db_error::internal;

        for(int attempt = 1; attempt <= retry_count; ++attempt){
            auto callback_result_exp = std::invoke(callback);
            if(callback_result_exp){
                return callback_result_exp;
            }

            const db_error callback_error = callback_result_exp.error();
            last_error = callback_error;
            if(
                attempt == retry_count ||
                !repository_error::should_retry_db_error(callback_error)
            ){
                return std::unexpected(callback_error);
            }

            if(repository_error::should_reconnect_db_error(callback_error)){
                const auto reconnect_exp = db_connection_value.reconnect();
                if(!reconnect_exp){
                    last_error = reconnect_exp.error();
                }
            }
        }

        return std::unexpected(last_error);
    }

    template <typename callback_type>
        requires expected_db_result<write_transaction_result<callback_type>>
    auto with_write_transaction(
        db_connection& connection,
        callback_type&& callback
    ) -> write_transaction_result<callback_type> {
        if(!connection.is_connected()){
            return std::unexpected(db_error::invalid_connection);
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
            return std::unexpected(
                db_error::from_error_code(error_code::map_psql_error_code(exception))
            );
        }
    }

    template <typename callback_type>
        requires expected_db_result<write_transaction_result<callback_type>>
    auto with_retry_write_transaction(
        db_connection& connection,
        int retry_count,
        callback_type&& callback
    ) -> write_transaction_result<callback_type> {
        return retry_db_operation(
            connection,
            retry_count,
            [&]() -> write_transaction_result<callback_type> {
                return with_write_transaction(connection, callback);
            }
        );
    }

    template <typename callback_type>
        requires expected_db_result<write_transaction_result<callback_type>>
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
        requires expected_service_result<service_write_transaction_result<callback_type>>
    auto with_service_write_transaction(
        db_connection& connection,
        callback_type&& callback
    ) -> service_write_transaction_result<callback_type> {
        if(!connection.is_connected()){
            return std::unexpected(
                map_db_error_to_service_error(db_error::invalid_connection)
            );
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
            return std::unexpected(
                map_db_error_to_service_error(
                    db_error::from_error_code(
                        error_code::map_psql_error_code(exception)
                    )
                )
            );
        }
    }

    template <typename callback_type>
        requires expected_service_result<service_write_transaction_result<callback_type>>
    auto with_retry_service_write_transaction(
        db_connection& connection,
        int retry_count,
        callback_type&& callback
    ) -> service_write_transaction_result<callback_type> {
        if(retry_count <= 0){
            return std::unexpected(
                map_db_error_to_service_error(db_error::invalid_argument)
            );
        }

        for(int attempt = 1; attempt <= retry_count; ++attempt){
            if(!connection.is_connected()){
                const auto retry_exp = detail::handle_service_db_error(
                    connection,
                    attempt,
                    retry_count,
                    db_error::invalid_connection
                );
                if(!retry_exp){
                    return std::unexpected(retry_exp.error());
                }
                continue;
            }

            try{
                pqxx::work transaction(connection.connection());
                auto callback_result_exp = std::invoke(callback, transaction);
                if(!callback_result_exp){
                    return callback_result_exp;
                }

                transaction.commit();
                return callback_result_exp;
            }
            catch(const std::exception& exception){
                const auto retry_exp = detail::handle_service_db_error(
                    connection,
                    attempt,
                    retry_count,
                    db_error::from_error_code(error_code::map_psql_error_code(exception))
                );
                if(!retry_exp){
                    return std::unexpected(retry_exp.error());
                }
            }
        }

        return std::unexpected(service_error::internal);
    }

    template <typename callback_type>
        requires expected_service_result<service_write_transaction_result<callback_type>>
    auto with_retry_service_write_transaction(
        db_connection& connection,
        callback_type&& callback
    ) -> service_write_transaction_result<callback_type> {
        return with_retry_service_write_transaction(
            connection,
            DB_TRANSACTION_ATTEMPT_COUNT,
            std::forward<callback_type>(callback)
        );
    }

    template <typename callback_type>
        requires expected_db_result<read_transaction_result<callback_type>>
    auto with_read_transaction(
        db_connection& connection,
        callback_type&& callback
    ) -> read_transaction_result<callback_type> {
        if(!connection.is_connected()){
            return std::unexpected(db_error::invalid_connection);
        }

        try{
            pqxx::read_transaction transaction(connection.connection());
            return std::invoke(std::forward<callback_type>(callback), transaction);
        }
        catch(const std::exception& exception){
            return std::unexpected(
                db_error::from_error_code(error_code::map_psql_error_code(exception))
            );
        }
    }

    template <typename callback_type>
        requires expected_db_result<read_transaction_result<callback_type>>
    auto with_retry_read_transaction(
        db_connection& connection,
        int retry_count,
        callback_type&& callback
    ) -> read_transaction_result<callback_type> {
        return retry_db_operation(
            connection,
            retry_count,
            [&]() -> read_transaction_result<callback_type> {
                return with_read_transaction(connection, callback);
            }
        );
    }

    template <typename callback_type>
        requires expected_db_result<read_transaction_result<callback_type>>
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

    template <typename callback_type>
        requires expected_service_result<service_read_transaction_result<callback_type>>
    auto with_service_read_transaction(
        db_connection& connection,
        callback_type&& callback
    ) -> service_read_transaction_result<callback_type> {
        if(!connection.is_connected()){
            return std::unexpected(
                map_db_error_to_service_error(db_error::invalid_connection)
            );
        }

        try{
            pqxx::read_transaction transaction(connection.connection());
            return std::invoke(std::forward<callback_type>(callback), transaction);
        }
        catch(const std::exception& exception){
            return std::unexpected(
                map_db_error_to_service_error(
                    db_error::from_error_code(
                        error_code::map_psql_error_code(exception)
                    )
                )
            );
        }
    }

    template <typename callback_type>
        requires expected_service_result<service_read_transaction_result<callback_type>>
    auto with_retry_service_read_transaction(
        db_connection& connection,
        int retry_count,
        callback_type&& callback
    ) -> service_read_transaction_result<callback_type> {
        if(retry_count <= 0){
            return std::unexpected(
                map_db_error_to_service_error(db_error::invalid_argument)
            );
        }

        for(int attempt = 1; attempt <= retry_count; ++attempt){
            if(!connection.is_connected()){
                const auto retry_exp = detail::handle_service_db_error(
                    connection,
                    attempt,
                    retry_count,
                    db_error::invalid_connection
                );
                if(!retry_exp){
                    return std::unexpected(retry_exp.error());
                }
                continue;
            }

            try{
                pqxx::read_transaction transaction(connection.connection());
                auto callback_result_exp = std::invoke(callback, transaction);
                if(!callback_result_exp){
                    return callback_result_exp;
                }

                return callback_result_exp;
            }
            catch(const std::exception& exception){
                const auto retry_exp = detail::handle_service_db_error(
                    connection,
                    attempt,
                    retry_count,
                    db_error::from_error_code(error_code::map_psql_error_code(exception))
                );
                if(!retry_exp){
                    return std::unexpected(retry_exp.error());
                }
            }
        }

        return std::unexpected(service_error::internal);
    }

    template <typename callback_type>
        requires expected_service_result<service_read_transaction_result<callback_type>>
    auto with_retry_service_read_transaction(
        db_connection& connection,
        callback_type&& callback
    ) -> service_read_transaction_result<callback_type> {
        return with_retry_service_read_transaction(
            connection,
            DB_TRANSACTION_ATTEMPT_COUNT,
            std::forward<callback_type>(callback)
        );
    }
}
