#include "common/env_utility.hpp"

#include <cstdlib>

std::expected<std::string, error_code> env_utility::require_env(const char* key){
    const char* value = std::getenv(key);
    if(value == nullptr || *value == '\0'){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return std::string(value);
}
