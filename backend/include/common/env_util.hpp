#pragma once
#include "error/infra_error.hpp"

#include <expected>
#include <initializer_list>
#include <string>
#include <vector>

namespace env_util{
    std::expected<std::string, infra_error> require_env(const char* key);
    std::expected<void, infra_error> require_http_server_envs();
    std::expected<void, infra_error> require_judge_server_envs();
    std::expected<void, infra_error> require_all_envs();
    std::expected<std::vector<std::string>, infra_error> require_envs(
        std::initializer_list<const char*> keys
    );
}
