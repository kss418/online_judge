#pragma once
#include "common/error_code.hpp"

#include <chrono>
#include <cstddef>
#include <expected>
#include <string>
#include <string_view>

namespace token_util{
    inline constexpr std::size_t TOKEN_BYTE_LENGTH = 32;
    inline constexpr std::chrono::seconds TOKEN_TTL{std::chrono::hours{24 * 30}};

    struct issued_token{
        std::string token;
        std::string token_hash;
    };

    std::expected<std::string, error_code> generate_token();
    std::expected<issued_token, error_code> issue_token();
}
