#pragma once

#include "common/error_code.hpp"

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
    bool should_retry_file_error(const error_code& error_code_value);

    template <typename result_type>
    concept expected_error_result = requires{
        typename result_type::value_type;
        typename result_type::error_type;
    } && std::same_as<typename result_type::error_type, error_code>;

    template <typename callback_type>
    using operation_result = std::invoke_result_t<callback_type>;

    template <typename callback_type>
        requires expected_error_result<operation_result<callback_type>>
    auto retry_file_operation(
        int retry_count,
        callback_type&& callback
    ) -> operation_result<callback_type> {
        if(retry_count <= 0){
            return std::unexpected(error_code::create(errno_error::invalid_argument));
        }

        error_code last_error = error_code::create(errno_error::unknown_error);

        for(int attempt = 1; attempt <= retry_count; ++attempt){
            auto callback_result_exp = std::invoke(callback);
            if(callback_result_exp){
                return callback_result_exp;
            }

            const error_code callback_error = callback_result_exp.error();
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

    std::expected<bool, error_code> exists(const std::filesystem::path& file_path);
    std::expected<void, error_code> create_directories(const std::filesystem::path& directory_path);
    std::expected<void, error_code> remove_file(const std::filesystem::path& file_path);
    std::expected<std::string, error_code> read_file_content(
        const std::filesystem::path& file_path
    );
    std::expected<std::int32_t, error_code> read_int32_file(const std::filesystem::path& file_path);

    std::expected<void, error_code> create_file(
        const std::filesystem::path& file_path,
        std::string_view file_content
    );
}
