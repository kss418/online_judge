#pragma once

#include "error/io_error.hpp"

#include <concepts>
#include <cstdint>
#include <chrono>
#include <expected>
#include <filesystem>
#include <functional>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <utility>

namespace file_util{
    bool should_retry_file_error(const io_error& error_code_value);

    template <typename result_type>
    concept expected_error_result = requires{
        typename result_type::value_type;
        typename result_type::error_type;
    } && std::same_as<typename result_type::error_type, io_error>;

    template <typename callback_type>
    using operation_result = std::invoke_result_t<callback_type>;

    template <typename callback_type>
        requires expected_error_result<operation_result<callback_type>>
    auto retry_file_operation(
        int retry_count,
        callback_type&& callback
    ) -> operation_result<callback_type> {
        if(retry_count <= 0){
            return std::unexpected(io_error::invalid_argument);
        }

        io_error last_error = io_error::internal;

        for(int attempt = 1; attempt <= retry_count; ++attempt){
            auto callback_result_exp = std::invoke(callback);
            if(callback_result_exp){
                return callback_result_exp;
            }

            const io_error callback_error = callback_result_exp.error();
            last_error = callback_error;
            if(
                attempt == retry_count ||
                !should_retry_file_error(callback_error)
            ){
                return std::unexpected(callback_error);
            }

            const int retry_delay_milliseconds = attempt < 5 ? attempt * 10 : 50;
            std::this_thread::sleep_for(
                std::chrono::milliseconds(retry_delay_milliseconds)
            );
        }

        return std::unexpected(last_error);
    }

    std::expected<bool, io_error> exists(const std::filesystem::path& file_path);
    std::expected<void, io_error> create_directories(const std::filesystem::path& directory_path);
    std::expected<void, io_error> remove_file(const std::filesystem::path& file_path);
    std::expected<void, io_error> remove_all(const std::filesystem::path& file_path);
    std::expected<std::string, io_error> read_file_content(
        const std::filesystem::path& file_path
    );
    std::expected<std::int32_t, io_error> read_int32_file(const std::filesystem::path& file_path);

    std::expected<void, io_error> create_file(
        const std::filesystem::path& file_path,
        std::string_view file_content
    );
}
