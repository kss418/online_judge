#pragma once
#include "common/error_code.hpp"

#include <expected>
#include <string>

namespace env_utility{
    std::expected<std::string, error_code> require_env(const char* key);
}
