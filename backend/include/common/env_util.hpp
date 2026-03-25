#pragma once
#include "common/error_code.hpp"

#include <expected>
#include <initializer_list>
#include <string>
#include <vector>

namespace env_util{
    std::expected<std::string, error_code> require_env(const char* key);
    std::expected<void, error_code> require_all_envs();
    std::expected<std::vector<std::string>, error_code> require_envs(
        std::initializer_list<const char*> keys
    );
}
