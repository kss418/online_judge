#pragma once
#include "common/error_code.hpp"

#include <expected>
#include <span>
#include <string>
#include <string_view>

namespace token_util{
    inline constexpr std::string_view HEX_DIGITS = "0123456789abcdef";
    inline constexpr std::size_t TOKEN_BYTE_LENGTH = 32;

    struct issued_token{
        std::string token;
        std::string token_hash;
    };

    std::string to_hex_string(std::span<const unsigned char> bytes);
    std::expected<std::string, error_code> generate_token();
    std::expected<issued_token, error_code> issue_token();
    std::expected<std::string, error_code> hash_token(std::string_view token);
}
