#pragma once

#include "common/db_connection.hpp"
#include "common/error_code.hpp"

#include <pqxx/pqxx>

#include <concepts>
#include <expected>
#include <functional>
#include <type_traits>
#include <utility>

namespace db_service_util{
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
}
