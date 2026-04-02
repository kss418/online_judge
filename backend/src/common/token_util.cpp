#include "common/token_util.hpp"
#include "common/crypto_util.hpp"

#include <openssl/rand.h>

#include <array>
#include <string>

std::expected<std::string, infra_error> token_util::generate_token(){
    std::array<unsigned char, TOKEN_BYTE_LENGTH> token_bytes{};
    const int rand_ok = RAND_bytes(
        token_bytes.data(),
        static_cast<int>(token_bytes.size())
    );
    if(rand_ok != 1){
        return std::unexpected(infra_error::internal);
    }

    return crypto_util::to_hex_string(token_bytes);
}

std::expected<token_util::issued_token, infra_error> token_util::issue_token(){
    const auto token_exp = generate_token();
    if(!token_exp){
        return std::unexpected(token_exp.error());
    }

    const auto token_hash_exp = crypto_util::sha512_hex(*token_exp);
    if(!token_hash_exp){
        return std::unexpected(token_hash_exp.error());
    }

    issued_token issued_token_value;
    issued_token_value.token = std::move(*token_exp);
    issued_token_value.token_hash = std::move(*token_hash_exp);
    return issued_token_value;
}
