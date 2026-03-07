#pragma once
#include "common/error_code.hpp"

#include <expected>
#include <initializer_list>
#include <string>

namespace env_utility{
    std::expected<std::string, error_code> require_env(const char* key);
    std::expected<void, error_code> require_all_envs();
    std::expected<void, error_code> require_envs(std::initializer_list<const char*> keys);
}
