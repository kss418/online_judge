#include "common/crypto_util.hpp"

#include <openssl/evp.h>

#include <array>
#include <span>
#include <string>

static constexpr std::string_view HEX_DIGITS = "0123456789abcdef";

std::string crypto_util::to_hex_string(std::span<const unsigned char> bytes){
    std::string hex_string;
    hex_string.reserve(bytes.size() * 2);
    for(const unsigned char byte : bytes){
        hex_string.push_back(HEX_DIGITS[byte >> 4]);
        hex_string.push_back(HEX_DIGITS[byte & 0x0f]);
    }

    return hex_string;
}

std::expected<std::string, error_code> crypto_util::sha512_hex(std::string_view value){
    if(value.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    std::array<unsigned char, EVP_MAX_MD_SIZE> digest{};
    unsigned int digest_length = 0;
    const int digest_ok = EVP_Digest(
        value.data(),
        value.size(),
        digest.data(),
        &digest_length,
        EVP_sha512(),
        nullptr
    );
    if(digest_ok != 1){
        return std::unexpected(error_code::create(errno_error::unknown_error));
    }

    return crypto_util::to_hex_string(
        std::span<const unsigned char>{digest.data(), digest_length}
    );
}
