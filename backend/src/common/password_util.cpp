#include "common/password_util.hpp"
#include "common/crypto_util.hpp"

std::expected<std::string, infra_error> password_util::get_password_hash(
    std::string_view password
){
    return crypto_util::sha512_hex(password);
}
