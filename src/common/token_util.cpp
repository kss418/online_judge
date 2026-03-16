#include "common/token_util.hpp"

#include <openssl/evp.h>
#include <openssl/rand.h>

#include <array>
#include <string>

std::string token_util::to_hex_string(std::span<const unsigned char> bytes){
    std::string hex_string;
    hex_string.reserve(bytes.size() * 2);
    for(const unsigned char byte : bytes){
        hex_string.push_back(HEX_DIGITS[byte >> 4]);
        hex_string.push_back(HEX_DIGITS[byte & 0x0f]);
    }

    return hex_string;
}

std::expected<std::string, error_code> token_util::generate_token(){
    std::array<unsigned char, TOKEN_BYTE_LENGTH> token_bytes{};
    const int rand_ok = RAND_bytes(
        token_bytes.data(),
        static_cast<int>(token_bytes.size())
    );
    if(rand_ok != 1){
        return std::unexpected(error_code::create(errno_error::unknown_error));
    }

    return to_hex_string(token_bytes);
}

std::expected<token_util::issued_token, error_code> token_util::issue_token(){
    const auto token_exp = generate_token();
    if(!token_exp){
        return std::unexpected(token_exp.error());
    }

    const auto token_hash_exp = hash_token(*token_exp);
    if(!token_hash_exp){
        return std::unexpected(token_hash_exp.error());
    }

    issued_token issued_token_value;
    issued_token_value.token = std::move(*token_exp);
    issued_token_value.token_hash = std::move(*token_hash_exp);
    return issued_token_value;
}

std::expected<std::string, error_code> token_util::hash_token(std::string_view token){
    if(token.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    std::array<unsigned char, EVP_MAX_MD_SIZE> digest{};
    unsigned int digest_length = 0;
    const int digest_ok = EVP_Digest(
        token.data(),
        token.size(),
        digest.data(),
        &digest_length,
        EVP_sha512(),
        nullptr
    );
    if(digest_ok != 1){
        return std::unexpected(error_code::create(errno_error::unknown_error));
    }

    return to_hex_string(std::span<const unsigned char>{digest.data(), digest_length});
}
