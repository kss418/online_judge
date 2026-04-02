#pragma once
#include "error/infra_error.hpp"

#include <expected>
#include <span>
#include <string>
#include <string_view>

namespace crypto_util{
    std::string to_hex_string(std::span<const unsigned char> bytes);
    std::expected<std::string, infra_error> sha512_hex(std::string_view value);
}
